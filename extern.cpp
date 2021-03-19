/**
 * Copyright (c) 2014-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "extern.hpp"

#include <folly/String.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <wdt/Receiver.h>
#include <wdt/Wdt.h>

#include <fstream>
#include <iostream>

using namespace facebook::wdt;

int receive() {
  const std::string FLAGS_app_name = "wdt";
  const std::string FLAGS_directory = ".";
  const std::string FLAGS_destination;

  Wdt &wdt = Wdt::initializeWdt(FLAGS_app_name);

  WdtOptions &options = wdt.getWdtOptions();

  ErrorCode retCode = OK;

  // General case : Sender or Receiver
  std::unique_ptr<WdtTransferRequest> reqPtr;
  reqPtr = std::make_unique<WdtTransferRequest>(
      options.start_port, options.num_ports, FLAGS_directory);
  WdtTransferRequest &req = *reqPtr;
  req.hostName = FLAGS_destination;
  req.transferId = "";
  req.ivChangeInterval = options.iv_change_interval_mb * kMbToB;
  req.tls = wdt.isTlsEnabled();
  req.wdtNamespace = "";

  Receiver receiver(req);
  WdtOptions &recOptions = receiver.getWdtOptions();
  wdt.wdtSetReceiverSocketCreator(receiver);
  WdtTransferRequest augmentedReq = receiver.init();
  retCode = augmentedReq.errorCode;
  if (retCode == FEWER_PORTS) {
    WLOG(ERROR) << "Receiver could not bind to all the ports";
    return FEWER_PORTS;
  } else if (augmentedReq.errorCode != OK) {
    WLOG(ERROR) << "Error setting up receiver " << errorCodeToStr(retCode);
    return retCode;
  }
  // In the log:
  WLOG(INFO) << "Starting receiver with connection url "
             << augmentedReq.getLogSafeString();  // The url without secret
  // on stdout: the one with secret:
  std::cout << augmentedReq.genWdtUrlWithSecret() << std::endl;
  std::cout.flush();
  retCode = receiver.transferAsync();
  if (retCode == OK) {
    std::unique_ptr<TransferReport> report = receiver.finish();
    retCode = report->getSummary().getErrorCode();
  }

  if (retCode == OK) {
    WLOG(INFO) << "Returning with OK exit code";
  } else {
    WLOG(ERROR) << "Returning with code " << retCode << " "
                << errorCodeToStr(retCode);
  }
  return retCode;
}
