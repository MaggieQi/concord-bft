// Concord
//
// Copyright (c) 2018 VMware, Inc. All Rights Reserved.
//
// This product is licensed to you under the Apache 2.0 license (the "License").
// You may not use this product except in compliance with the Apache 2.0
// License.
//
// This product may include a number of subcomponents with separate copyright
// notices and license terms. Your use of these subcomponents is subject to the
// terms and conditions of the subcomponent's license, as noted in the LICENSE
// file.

#ifndef CONCORD_BFT_TEST_PARAMETERS_HPP
#define CONCORD_BFT_TEST_PARAMETERS_HPP

struct ClientParams {
  uint32_t numOfOperations = 2800;
  uint16_t clientId = 4;
  uint16_t numOfReplicas = 4;
  uint16_t numOfClients = 1;
  uint16_t numOfFaulty = 1;
  uint16_t numOfSlow = 0;
  std::string   configFileName;

  std::string protocol = "concord";
  uint16_t maxBatchSize = 1;
  uint16_t payloadBytes = 8;

  uint16_t get_numOfReplicas() {
    return (uint16_t)(3 * numOfFaulty + 2 * numOfSlow + 1);
  }
};

struct ReplicaParams {
  uint16_t replicaId;
  uint16_t numOfReplicas = 4;
  uint16_t numOfClients = 1;
  bool debug = false;
  bool viewChangeEnabled = false;
  uint32_t viewChangeTimeout = 60000; // ms
  uint16_t statusReportTimerMillisec = 20 * 1000; // ms
  std::string   configFileName;
  std::string   keysFilePrefix;

  std::string protocol = "concord";
  std::string env = "local";
  bool dynamicCollectorEnabled = true;
  uint16_t concurrencyLevel = 1;
  uint16_t maxBatchSize = 1;
  uint16_t commitTimerMillisec = 0;
  uint16_t commitDelayMillisec = 10;

  uint32_t stopAtSec = 0xfffffffe;
  uint32_t listenThreads = 1;
};

#endif //CONCORD_BFT_TEST_PARAMETERS_HPP
