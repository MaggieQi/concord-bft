// Concord
//
// Copyright (c) 2018 VMware, Inc. All Rights Reserved.
//
// This product is licensed to you under the Apache 2.0 license (the "License").
// You may not use this product except in compliance with the Apache 2.0 License.
//
// This product may include a number of subcomponents with separate copyright
// notices and license terms. Your use of these subcomponents is subject to the
// terms and conditions of the subcomponent's license, anoted in the LICENSE file.

#include <unordered_map>
#include <string>
#include <functional>
#include <iostream>
#include <sstream>
#include <thread>
#include <string.h>
#include <chrono>
#include <mutex>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <crtdbg.h>
#else

#include <execinfo.h>
#include <unistd.h>
#include <sys/time.h>

#endif

#include "CommDefs.hpp"
#include "Logger.hpp"
#include "boost/bind.hpp"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/move/unique_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/make_unique.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/noncopyable.hpp>

class AsyncTcpConnection;

using namespace std;
using namespace bftEngine;
using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::posix_time;

using boost::asio::io_service;
using boost::system::error_code;
using boost::asio::ip::address;

typedef boost::system::error_code B_ERROR_CODE;
typedef boost::shared_ptr<AsyncTcpConnection> ASYNC_CONN_PTR;
typedef tcp::socket B_TCP_SOCKET;

// first 4 bytes - message length, next 2 bytes - message type
static constexpr uint8_t LENGTH_FIELD_SIZE = 4;
static constexpr uint8_t MSGTYPE_FIELD_SIZE = 2;

enum MessageType : uint16_t {
  Reserved = 0,
  Hello,
  Regular
};

enum ConnType : uint8_t {
  Incoming,
  Outgoing
};


class io_service_pool
  : private boost::noncopyable
{
public:
  explicit io_service_pool(std::size_t pool_size): num_threads_(pool_size), next_io_service_(0) {
    for (std::size_t i = 0; i < pool_size; i++) {
      io_service_ptr io_(new io_service);
      work_ptr w_(new boost::asio::io_service::work(*io_));
      io_services_.push_back(io_);
      work_.push_back(w_);
    }
  }

  void run() {
    for (std::size_t i = 0; i < num_threads_; ++i)
      threads_.create_thread(boost::bind(&boost::asio::io_service::run, io_services_[i]));
      //threads_.create_thread(std::bind(static_cast<size_t(boost::asio::io_service::*)()>(&boost::asio::io_service::run), io_services_[i]));
  }

  void stop() {
    for (std::size_t i = 0; i < io_services_.size(); ++i)
      io_services_[i]->stop();

    threads_.join_all();
  }

  boost::asio::io_service& get_io_service() {
    io_service& io_ = *io_services_[next_io_service_++];
    if (next_io_service_ == io_services_.size()) next_io_service_ = 0;
    return io_;
  }

private:
  typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
  typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

  /// The pool of io_services.
  std::vector<io_service_ptr> io_services_;

  /// The work that keeps the io_services running.
  std::vector<work_ptr> work_;

  boost::thread_group threads_;

  std::size_t num_threads_;

  /// The next io_service to use for a connection.
  std::size_t next_io_service_;
};

/** this class will handle single connection using boost::make_shared idiom
 * will receive the IReceiver as a parameter and call it when new message
 * is available
 * TODO(IG): add multithreading
 */
class AsyncTcpConnection :
    public boost::enable_shared_from_this<AsyncTcpConnection> {
 private:
  bool _isReplica = false;
  bool _destIsReplica = false;
  io_service *_service = nullptr;
  uint32_t _bufferLength;
  char *_inBuffer = nullptr;
  char *_outBuffer = nullptr;
  IReceiver *_receiver = nullptr;
  function<void(NodeNum)> _fOnError = nullptr;
  function<void(NodeNum, ASYNC_CONN_PTR)> _fOnHellOMessage = nullptr;
  NodeNum _destId;
  NodeNum _selfId;
  string _ip = "";
  uint16_t _port = 0;
  deadline_timer _connectTimer;
  ConnType _connType;
  bool _closed;
  concordlogger::Logger _logger;
  uint16_t _minTimeout = 256;
  uint16_t _maxTimeout = 8192;
  uint16_t _currentTimeout = _minTimeout;
  bool _wasError = false;
  bool _connecting = false;
  UPDATE_CONNECTIVITY_FN _statusCallback = nullptr;
  NodeMap *_nodes;
  recursive_mutex _connectionsGuard;

 public:
  B_TCP_SOCKET socket;
  bool connected;

 private:
  AsyncTcpConnection(io_service *service,
                     function<void(NodeNum)> onError,
                     function<void(NodeNum, ASYNC_CONN_PTR)> onHelloMsg,
                     uint32_t bufferLength,
                     NodeNum destId,
                     NodeNum selfId,
                     ConnType type,
                     const concordlogger::Logger& logger,
                     UPDATE_CONNECTIVITY_FN statusCallback,
                     NodeMap *nodes) :
      _service(service),
      _bufferLength(bufferLength),
      _fOnError(onError),
      _fOnHellOMessage(onHelloMsg),
      _destId(destId),
      _selfId(selfId),
      _connectTimer(*service),
      _connType(type),
      _closed(false),
      _logger(logger),
      _statusCallback{statusCallback},
      _nodes{nodes},
      socket(*service),
      connected(false) {

    LOG_TRACE(_logger, "enter, node " << _selfId << ", dest: " << _destId);

    _isReplica = check_replica(_selfId);
    _inBuffer = new char[bufferLength];
    _outBuffer = new char[bufferLength];
 
    _connectTimer.expires_at(boost::posix_time::pos_infin);

    LOG_TRACE(_logger, "exit, node " << _selfId << ", dest: " << _destId);
  }

  bool check_replica(NodeNum node) {
    auto it = _nodes->find(node);
    if (it == _nodes->end()) {
      return false;
    }

    return it->second.isReplica;
  }

  void parse_message_header(const char *buffer,
                            uint32_t &msgLength) {
    msgLength = *(static_cast<const uint32_t *>(
        static_cast<const void *>(buffer)));
  }

  void close_socket() {
    LOG_TRACE(_logger, "enter, node " << _selfId
              << ", dest: " << _destId
              << ", connected: " << connected
              << ", closed: " << _closed);

    try {
      boost::system::error_code ignored_ec;
      socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                      ignored_ec);
      socket.close();
    } catch (std::exception &e) {
      LOG_ERROR(_logger, "exception, node " << _selfId
               << ", dest: " << _destId
               << ", connected: " << connected
               << ", ex: " << e.what());
    }

    LOG_TRACE(_logger, "exit, node " << _selfId
              << ", dest: " << _destId
              << ", connected: " << connected
              << ", closed: " << _closed);
  }

  bool was_error(const B_ERROR_CODE &ec, string where) {
    if (ec)
      LOG_ERROR(_logger, "where: " << where
                << ", node " << _selfId
                << ", dest: " << _destId
                << ", connected: " << connected
                << ", ex: " << ec.message());

    return (ec != 0);
  }

  void reconnect() {
    _connecting = true;

    LOG_INFO(_logger, "enter reconnect, node " << _selfId
               << ", dest: " << _destId
               << ", connected: " << connected
               << "is_open: " << socket.is_open());

    lock_guard<recursive_mutex> lock(_connectionsGuard);

    connected = false;
    close_socket();

    socket = B_TCP_SOCKET(*_service);

    setTimeOut();
    connect(_ip, _port, _destIsReplica);

    LOG_TRACE(_logger, "exit, node " << _selfId
              << ", dest: " << _destId
              << ", connected: " << connected
              << "is_open: " << socket.is_open());
  }

  void handle_error(B_ERROR_CODE ec) {
    if (boost::asio::error::operation_aborted == ec) {
      return;
    }

    if (ConnType::Incoming == _connType) {
      close();
    }
    else {
      reconnect();
      if (_statusCallback) {
        bool isReplica = check_replica(_selfId);
        if (isReplica) {
          PeerConnectivityStatus pcs{};
          pcs.peerId = _selfId;
          pcs.statusType = StatusType::Broken;

          // pcs.statusTime = we dont set it since it is set by the aggregator
          // in the upcoming version timestamps should be reviewed
          _statusCallback(pcs);
        }
      }
    }
  }

  void
  read_header_async_completed(const B_ERROR_CODE &ec,
                              const uint32_t bytesRead) {
    LOG_TRACE(_logger, "enter, node " << _selfId
              << ", dest: " << _destId
              << ", connected: " << connected
              << "is_open: " << socket.is_open());
    bool err;
    {
      lock_guard<recursive_mutex> lock(_connectionsGuard);

      if (_wasError || _connecting) {
        LOG_TRACE(_logger,
            "was error, node " << _selfId << ", dest: " << _destId);
        return;
      }

      err = was_error(ec, __func__);
      if (err) {
        handle_error(ec);
      } else {
        uint32_t msgLength;
        parse_message_header(_inBuffer, msgLength);
        if (msgLength == 0) {
          LOG_ERROR(_logger, "on_read_async_header_completed, msgLen=0");
          return;
        }

        read_msg_async(LENGTH_FIELD_SIZE, msgLength);

        LOG_TRACE(_logger, "exit, node " << _selfId
                  << ", dest: " << _destId
                  << ", connected: " << connected
                  << "is_open: " << socket.is_open());
      }
    }
    if (err && _closed) _fOnError(_destId);
  }

  void read_header_async() {
    LOG_TRACE(_logger, "enter, node " << _selfId
              << ", dest: " << _destId
              << ", connected: " << connected
              << "is_open: " << socket.is_open());

    memset(_inBuffer, 0, _bufferLength);
    async_read(socket,
               buffer(_inBuffer, LENGTH_FIELD_SIZE),
               boost::bind(&AsyncTcpConnection::read_header_async_completed,
                           shared_from_this(),
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred));

    LOG_TRACE(_logger, "exit, node " << _selfId
              << ", dest: " << _destId
              << ", connected: " << connected
              << "is_open: " << socket.is_open());
  }

  bool is_service_message() {
    uint16_t msgType =
        *(static_cast<uint16_t *>(
            static_cast<void *>(_inBuffer + LENGTH_FIELD_SIZE)));
    switch (msgType) {
      case MessageType::Hello:
        _destId =
            *(static_cast<NodeNum *>(
                static_cast<void *>(
                    _inBuffer + LENGTH_FIELD_SIZE +
                        MSGTYPE_FIELD_SIZE)));

        LOG_DEBUG(_logger, "node: " << _selfId << " got hello from:" << _destId);

        _destIsReplica = check_replica(_destId);
        LOG_DEBUG(_logger, "node: " << _selfId
                  << " dest is replica: " << _destIsReplica);
        return true;
      default:return false;
    }
  }

  void read_msg_async_completed(const boost::system::error_code &ec,
                                size_t bytesRead) {
    LOG_TRACE(_logger, "enter, node " << _selfId << ", dest: " << _destId);
    bool ishello = false;
    {
      lock_guard<recursive_mutex> lock(_connectionsGuard);

      if (_wasError || _connecting) {
        LOG_TRACE(_logger,
            "was error, node " << _selfId << ", dest: " << _destId);
        return;
      }

      auto err = was_error(ec, __func__);
      if (err) {
        _wasError = true;
        return;
      }

      if (!is_service_message()) {
        LOG_DEBUG(_logger, "data msg received, msgLen: " << bytesRead);
        _receiver->
            onNewMessage(_destId,
                        _inBuffer + LENGTH_FIELD_SIZE +
                            MSGTYPE_FIELD_SIZE,
                        bytesRead - MSGTYPE_FIELD_SIZE);
      } else {
        ishello = true;
      }

      read_header_async();

      if (_statusCallback && _destIsReplica) {
        PeerConnectivityStatus pcs{};
        pcs.peerId = _destId;
        pcs.peerIp = _ip;
        pcs.peerPort = _port;
        pcs.statusType = StatusType::MessageReceived;

        // pcs.statusTime = we dont set it since it is set by the aggregator
        // in the upcoming version timestamps should be reviewed
        _statusCallback(pcs);
      }

      LOG_TRACE(_logger, "exit, node " << _selfId << ", dest: " << _destId);
    }
    if (ishello) _fOnHellOMessage(_destId, shared_from_this());
  }

  void read_msg_async(uint32_t offset, uint32_t msgLength) {
    LOG_TRACE(_logger, "enter, node " << _selfId << ", dest: " << _destId);

    // async operation will finish when either expectedBytes are read
    // or error occured
    async_read(socket,
               boost::asio::buffer(_inBuffer + offset,
                                   msgLength),
               boost::bind(&AsyncTcpConnection::read_msg_async_completed,
                           shared_from_this(),
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred));

    LOG_TRACE(_logger, "exit, node " << _selfId << ", dest: " << _destId);
  }

  void write_async_completed(const B_ERROR_CODE &err,
                             size_t bytesTransferred) {
    LOG_TRACE(_logger, "enter, node " << _selfId << ", dest: " << _destId);

    if (_wasError) {
      LOG_TRACE(_logger,
          "was error, node " << _selfId << ", dest: " << _destId);
      return;
    }

    auto res = was_error(err, __func__);

    if (res) {
      _wasError = true;
      return;
    }

    LOG_TRACE(_logger, "exit, node " << _selfId << ", dest: " << _destId);
  }

  uint16_t prepare_output_buffer(uint16_t msgType, uint32_t dataLength) {
    memset(_outBuffer, 0, _bufferLength);
    uint32_t size = sizeof(msgType) + dataLength;
    memcpy(_outBuffer, &size, LENGTH_FIELD_SIZE);
    memcpy(_outBuffer + LENGTH_FIELD_SIZE,
           &msgType,
           MSGTYPE_FIELD_SIZE);
    return LENGTH_FIELD_SIZE + MSGTYPE_FIELD_SIZE;
  }

  bool send_hello() {
    auto offset = prepare_output_buffer(MessageType::Hello, sizeof(_selfId));
    memcpy(_outBuffer + offset, &_selfId, sizeof(_selfId));

    LOG_DEBUG(_logger, "sending hello from:" << _selfId
              << " to: " << _destId
              << ", size: " << (offset + sizeof(_selfId)));

    return AsyncTcpConnection::write_async((const char *) _outBuffer,
                                    offset + sizeof(_selfId));
  }

  void setTimeOut() {
    _currentTimeout = _currentTimeout == _maxTimeout
                      ? _minTimeout
                      : _currentTimeout * 2;
  }

  void connect_timer_tick(const B_ERROR_CODE &ec) {
    LOG_TRACE(_logger, "enter, node " << _selfId <<
                                      ", dest: " << _destId << ", ec: "
                                      << ec.message());

    if (_closed) {
      LOG_DEBUG(_logger,
                "closed, node " << _selfId << ", dest: " << _destId << ", ec: "
                                << ec.message());
    } else {
      if (connected) {
        LOG_DEBUG(_logger, "already connected, node " << _selfId
                  << ", dest: " << _destId
                  << ", ec: " << ec);
        _connectTimer.expires_at(boost::posix_time::pos_infin);
      } else if (_connectTimer.expires_at() <=
          deadline_timer::traits_type::now()) {

        LOG_DEBUG(_logger, "reconnecting, node " << _selfId
                  << ", dest: " << _destId
                  << ", ec: " << ec);
        reconnect();
      } else {
        LOG_DEBUG(_logger, "else, node " << _selfId
                  << ", dest: " << _destId
                  << ", ec: " << ec.message());
      }

      _connectTimer.async_wait(
          boost::bind(&AsyncTcpConnection::connect_timer_tick,
                      shared_from_this(),
                      boost::asio::placeholders::error));
    }

    LOG_TRACE(_logger, "exit, node " << _selfId
              <<", dest: " << _destId
              << ", ec: " << ec.message());
  }

  void connect_completed(const B_ERROR_CODE &err) {
    LOG_TRACE(_logger, "enter, node " << _selfId << ", dest: " << _destId);
    
    lock_guard<recursive_mutex> lock(_connectionsGuard);
    auto res = was_error(err, __func__);

    if (!socket.is_open()) {
      // async_connect opens socket on start so
      //nothing to do here since timeout occured and closed the socket
      if (connected) {
        LOG_DEBUG(_logger, "node " << _selfId
                  << " is DISCONNECTED from node " << _destId);
      }
      connected = false;
    } else if (res) {
      connected = false;
      //timeout didnt happen yet but the connection failed
      // nothig to do here, left for clarity
    } else {
      LOG_DEBUG(_logger, "connected, node " << _selfId
                << ", dest: " << _destId
                << ", res: " << res);

      connected = true;
      _wasError = false;
      _connecting = false;
      _connectTimer.expires_at(boost::posix_time::pos_infin);
      _currentTimeout = _minTimeout;
      send_hello();
      read_header_async();
    }
    
    LOG_TRACE(_logger, "exit, node " << _selfId << ", dest: " << _destId);
  }

  bool write_async(const char *data, uint32_t length) {
    if (!connected)
      return false;

    B_ERROR_CODE ec;
    write(socket, buffer(data, length), ec);
    auto err = was_error(ec, __func__);
    if (err) {
      handle_error(ec);
    }
    return !err;
  }

  void init() {
    _connectTimer.async_wait(
        boost::bind(&AsyncTcpConnection::connect_timer_tick,
                    shared_from_this(),
                    boost::asio::placeholders::error));
  }

 public:
  void connect(string ip, uint16_t port, bool destIsReplica) {
    _ip = ip;
    _port = port;
    _destIsReplica = destIsReplica;

    LOG_TRACE(_logger, "enter, from: " << _selfId
              << " ,to: " << _destId
              << ", ip: " << ip
              << ", port: " << port);

    tcp::endpoint ep(address::from_string(ip), port);
    LOG_DEBUG(_logger, "connecting from: " << _selfId
              << " ,to: " << _destId
              << ", timeout: " << _currentTimeout
              << ", dest is replica: " << _destIsReplica);

    _connectTimer.expires_from_now(
        boost::posix_time::millisec(_currentTimeout));

    socket.async_connect(ep,
                         boost::bind(&AsyncTcpConnection::connect_completed,
                                     shared_from_this(),
                                     boost::asio::placeholders::error));
    LOG_TRACE(_logger, "exit, from: " << _selfId
             << " ,to: " << _destId
             << ", ip: " << ip
             << ", port: " << port);
  }

  void start() {
    read_header_async();
  }

  void close() {
    _connecting = true;
    LOG_TRACE(_logger, "enter, node " << _selfId
              << ", dest: " << _destId
              << ", connected: " << connected
              << ", closed: " << _closed);

    lock_guard<recursive_mutex> lock(_connectionsGuard);

    connected = false;
    _closed = true;
    _connectTimer.cancel();

    try {
      B_ERROR_CODE ec;
      socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
      socket.close();
    } catch (std::exception &e) {
      LOG_ERROR(_logger, "exception, node " << _selfId
                << ", dest: " << _destId
                << ", connected: " << connected
                << ", ex: " << e.what());
    }

    LOG_TRACE(_logger, "exit, node " << _selfId
              << ", dest: " << _destId
              << ", connected: " << connected
              << ", closed: " << _closed);
  }

  void send(const char *data, uint32_t length) {
    LOG_TRACE(_logger, "enter, node " << _selfId << ", dest: " << _destId);
    bool err;
    {
      lock_guard<recursive_mutex> lock(_connectionsGuard);
      auto offset = prepare_output_buffer(MessageType::Regular,
                                          length);
      memcpy(_outBuffer + offset, data, length);
      err = !write_async(_outBuffer, offset + length);

      if (_statusCallback && _isReplica) {
        PeerConnectivityStatus pcs{};
        pcs.peerId = _selfId;
        pcs.statusType = StatusType::MessageSent;

        // pcs.statusTime = we dont set it since it is set by the aggregator
        // in the upcoming version timestamps should be reviewed
        _statusCallback(pcs);
      }

      LOG_DEBUG(_logger, "send exit, from: " << ", to: " << _destId
                << ", offset: " << offset
                << ", length: " << length);
      LOG_TRACE(_logger, "exit, node " << _selfId << ", dest: " << _destId);
    }
    if (err && _closed) _fOnError(_destId);
  }

  static ASYNC_CONN_PTR create(io_service *service,
                               function<void(NodeNum)> onError,
                               function<void(NodeNum, ASYNC_CONN_PTR)> onHello,
                               uint32_t bufferLength,
                               NodeNum destId,
                               NodeNum selfId,
                               ConnType type,
                               const concordlogger::Logger& logger,
                               UPDATE_CONNECTIVITY_FN statusCallback,
                               NodeMap* nodes) {
    auto res = ASYNC_CONN_PTR(
        new AsyncTcpConnection(service,
                               onError,
                               onHello,
                               bufferLength,
                               destId,
                               selfId,
                               type,
                               logger,
                               statusCallback,
                               nodes));
    res->init();
    return res;
  }

  void setReceiver(IReceiver *rec) {
    _receiver = rec;
  }

  virtual ~AsyncTcpConnection() {
    LOG_TRACE(_logger, "enter, node " << _selfId
              << ", dest: " << _destId
              << ", connected: " << connected
              << ", closed: " << _closed);

    delete[] _inBuffer;
    delete[] _outBuffer;

    LOG_TRACE(_logger, "exit, node " << _selfId
              << ", dest: " << _destId
              << ", connected: " << connected
              << ", closed: " << _closed);
  }
};

///////////////////////////////////////////////////////////////////////////////

class PlainTCPCommunication::PlainTcpImpl {
 private:
  unordered_map<NodeNum, ASYNC_CONN_PTR> _connections;
  concordlogger::Logger _logger = concordlogger::Log::getLogger
      ("concord-bft.tcp");

  unique_ptr<tcp::acceptor> _pAcceptor;
  boost::thread_group *_pIoThread = nullptr;

  NodeNum _selfId;
  IReceiver *_pReceiver;

  io_service_pool _service;
  io_service _replicaService;
  uint16_t _listenPort;
  string _listenIp;
  uint32_t _bufferLength;
  uint32_t _maxServerId;
  UPDATE_CONNECTIVITY_FN _statusCallback = nullptr;
  recursive_mutex _connectionsGuard;
  NodeMap _nodes;
  uint32_t _numConnections;

  void on_async_connection_error(NodeNum peerId) {
    LOG_ERROR(_logger, "to: " << peerId);
    lock_guard<recursive_mutex> lock(_connectionsGuard);
    _connections.erase(peerId);
  }

  void on_hello_message(NodeNum id, ASYNC_CONN_PTR conn) {
    //LOG_TRACE(_logger, "node: " << _selfId << ", from: " << id);
    LOG_INFO(_logger, "node: " << _selfId << ", receive hello from: " << id << ", numConnections:" << _numConnections);

    //* potential fix for segment fault *//
    lock_guard<recursive_mutex> lock(_connectionsGuard);
    _connections.insert(make_pair(id, conn));
    conn->setReceiver(_pReceiver);
  }

  void on_accept(ASYNC_CONN_PTR conn,
                 const B_ERROR_CODE &ec) {
    LOG_TRACE(_logger, "enter, node: " << _selfId << ", ec: " << ec.message());

    if (!ec) {
      conn->connected = true;
      conn->start();
    } else {
      conn->close();
    }

    start_accept();
    LOG_TRACE(_logger, "exit, node: " << _selfId << "ec: " << ec.message());
  }

  //here need to check how "this" passed to handlers behaves
  // if the object is deleted.
  void start_accept() {
    LOG_TRACE(_logger, "enter, node: " << _selfId);

    auto conn = AsyncTcpConnection::
    create((_numConnections++ < _maxServerId)? &_replicaService: &(_service.get_io_service()),
           std::bind(
               &PlainTcpImpl::on_async_connection_error,
               this,
               std::placeholders::_1),
           std::bind(
               &PlainTcpImpl::on_hello_message,
               this,
               std::placeholders::_1,
               std::placeholders::_2),
           _bufferLength,
           0,
           _selfId,
           ConnType::Incoming,
           _logger,
           _statusCallback,
           &_nodes);
    _pAcceptor->async_accept(conn->socket,
                             boost::bind(
                                 &PlainTcpImpl::on_accept,
                                 this,
                                 conn,
                                 boost::asio::placeholders::error));
    LOG_TRACE(_logger, "exit, node: " << _selfId);
  }

  PlainTcpImpl(const PlainTcpImpl &) = delete; // non construction-copyable
  PlainTcpImpl(const PlainTcpImpl &&) = delete; // non movable
  PlainTcpImpl &operator=(const PlainTcpImpl &) = delete; // non copyable
  PlainTcpImpl() = delete;

  PlainTcpImpl(NodeNum selfNodeId,
               const NodeMap& nodes,
               uint32_t bufferLength,
               uint16_t listenPort,
               uint32_t maxServerId,
               string listenIp,
               UPDATE_CONNECTIVITY_FN statusCallback,
               uint32_t listenThreads) :
      _selfId{selfNodeId},
      _service{listenThreads % 100},
      _listenPort{listenPort},
      _listenIp{listenIp},
      _bufferLength{bufferLength},
      _maxServerId{maxServerId},
      _statusCallback{statusCallback},
      _nodes{nodes},
      _numConnections{listenThreads >= 100? 65536 : 0 } {

    // all replicas are in listen mode
    if (_selfId <= _maxServerId) {
      LOG_DEBUG(_logger, "node " << _selfId << " listening on " << _listenPort);
      tcp::endpoint ep(address::from_string(_listenIp), _listenPort);
      _pAcceptor = boost::make_unique<tcp::acceptor>(_service.get_io_service(), ep);
      _pAcceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
      start_accept();
    } else // clients dont need to listen
      LOG_INFO(_logger, "skipping listen for node: " << _selfId);

    // this node should connect only to nodes with lower ID
    // and all nodes with higher ID will connect to this node
    // we dont want that clients will connect to another clients
    for (auto it = _nodes.begin(); it != _nodes.end(); it++) {
      if (it->first < _selfId && it->first <= maxServerId) {
        auto conn =
            AsyncTcpConnection::
            create((_selfId <= _maxServerId && _numConnections++ < _maxServerId)? &_replicaService: &(_service.get_io_service()),
                   std::bind(
                       &PlainTcpImpl::on_async_connection_error,
                       this,
                       std::placeholders::_1),
                   std::bind(
                       &PlainTcpImpl::on_hello_message,
                       this,
                       std::placeholders::_1,
                       std::placeholders::_2),
                   _bufferLength,
                   it->first,
                   _selfId,
                   ConnType::Outgoing,
                   _logger,
                   _statusCallback,
                   &_nodes);

        _connections.insert(make_pair(it->first, conn));
        string peerIp = it->second.ip;
        uint16_t peerPort = it->second.port;
        conn->connect(peerIp, peerPort, it->second.isReplica);
        LOG_TRACE(_logger, "connect called for node " << to_string(it->first));
      }
      if (it->second.isReplica && _statusCallback) {
        PeerConnectivityStatus pcs{};
        pcs.peerId = it->first;
        pcs.peerIp = it->second.ip;
        pcs.peerPort = it->second.port;
        pcs.statusType = StatusType::Started;

        // pcs.statusTime = we dont set it since it is set by the aggregator
        // in the upcoming version timestamps should be reviewed
        _statusCallback(pcs);
      }
    }
  }


 public:
  static PlainTcpImpl *
  create(NodeNum selfNodeId,
      // tuple of ip, listen port, bind port
         const NodeMap& nodes,
         uint32_t bufferLength,
         uint16_t listenPort,
         uint32_t tempHighestNodeForConnecting,
         string listenIp,
         UPDATE_CONNECTIVITY_FN statusCallback,
         uint32_t listenThreads) {
    return new PlainTcpImpl(selfNodeId,
                            nodes,
                            bufferLength,
                            listenPort,
                            tempHighestNodeForConnecting,
                            listenIp,
                            statusCallback,
                            listenThreads);
  }

  int Start() {
    if (_pIoThread)
      return 0; // running

    _pIoThread = new boost::thread_group;    
    _service.run();
    if (_selfId <= _maxServerId && _numConnections < 65536)
      _pIoThread->create_thread(std::bind(static_cast<size_t(boost::asio::io_service::*)()>(&boost::asio::io_service::run), std::ref(_replicaService)));
    return 0;
  }

  /**
  * Stops the object (including its internal threads).
  * On success, returns 0.
  */
  int Stop() {
    if (!_pIoThread)
      return 0; // stopped

    _service.stop();
    _replicaService.stop();

    _pIoThread->join_all();

    _connections.clear();

    return 0;
  }

  bool isRunning() const {
    if (!_pIoThread)
      return false; // stopped
    return true;
  }

  ConnectionStatus
  getCurrentConnectionStatus(const NodeNum node) const {
    return isRunning() ?
           ConnectionStatus::Connected :
           ConnectionStatus::Disconnected;
  }

  /**
  * Sends a message on the underlying communication layer to a given
  * destination node. Asynchronous (non-blocking) method.
  * Returns 0 on success.
  */
  int sendAsyncMessage(const NodeNum destNode,
                       const char *const message,
                       const size_t messageLength) {
    LOG_TRACE(_logger, "enter, from: " << _selfId
              << ", to: " << to_string(destNode));

    lock_guard<recursive_mutex> lock(_connectionsGuard);
    auto temp = _connections.find(destNode);
    if (temp != _connections.end()) {
      LOG_TRACE(_logger, "conncection found, from: " << _selfId
                << ", to: " << destNode);

      if (temp->second->connected) {
        temp->second->send(message, messageLength);
      } else {
        LOG_TRACE(_logger,
           "conncection found but disconnected, from: " << _selfId
           << ", to: " << destNode);
      }
    }

    LOG_TRACE(_logger, "exit, from: " << _selfId
              << ", to: " << destNode);

    return 0;
  }

  /// TODO(IG): return real max message size... what is should be for TCP?
  int getMaxMessageSize() {
    return -1;
  }

  void setReceiver(NodeNum receiverNum, IReceiver *receiver) {
    _pReceiver = receiver;
    for (auto conn : _connections) {
      conn.second->setReceiver(receiver);
    }
  }

  virtual ~PlainTcpImpl() {
    LOG_TRACE(_logger, "PlainTCPDtor");
    delete _pIoThread;
    _pIoThread = nullptr;
  }
};

PlainTCPCommunication::~PlainTCPCommunication() {
  if (_ptrImpl) {
    delete _ptrImpl;
  }
}

PlainTCPCommunication::PlainTCPCommunication(const PlainTcpConfig &config) {
  _ptrImpl = PlainTcpImpl::create(config.selfId,
                                  config.nodes,
                                  config.bufferLength,
                                  config.listenPort,
                                  config.maxServerId,
                                  config.listenIp,
                                  config.statusCallback,
                                  config.listenThreads);
}

PlainTCPCommunication *PlainTCPCommunication::create(
    const PlainTcpConfig &config) {
  return new PlainTCPCommunication(config);
}

int PlainTCPCommunication::getMaxMessageSize() {
  return _ptrImpl->getMaxMessageSize();
}

int PlainTCPCommunication::Start() {
  return _ptrImpl->Start();
}

int PlainTCPCommunication::Stop() {
  if (!_ptrImpl)
    return 0;

  auto res = _ptrImpl->Stop();
  return res;
}

bool PlainTCPCommunication::isRunning() const {
  return _ptrImpl->isRunning();
}

ConnectionStatus
PlainTCPCommunication::getCurrentConnectionStatus(const NodeNum node) const {
  return _ptrImpl->getCurrentConnectionStatus(node);
}

int
PlainTCPCommunication::sendAsyncMessage(const NodeNum destNode,
                                        const char *const message,
                                        const size_t messageLength) {
  return _ptrImpl->sendAsyncMessage(destNode, message, messageLength);
}

void
PlainTCPCommunication::setReceiver(NodeNum receiverNum, IReceiver *receiver) {
  _ptrImpl->setReceiver(receiverNum, receiver);
}
