/**
 * @file TestLc.cpp
 * @author hc (you@domain.com)
 * @brief Test Additive Counter Shaing
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "TestDwayLazyReset.h"
#include <getopt.h>
#include <iostream>

using namespace OmniSketch;

static void Help(const char *ptr);

// Main
int main(int argc, char *argv[]) {
  std::string config_file = "../src/lc_config.toml";

  // parse command line arguments
  int opt;
  option options[] = {{"config", required_argument, nullptr, 'c'},
                      {"help", no_argument, nullptr, 'h'},
                      {"verbose", no_argument, nullptr, 'v'}};
  while ((opt = getopt_long(argc, argv, "c:hv", options, nullptr)) != -1) {
    switch (opt) {
    case 'c':
      config_file = optarg;
      break;
    case 'h':
      Help(argv[0]); // never return
      break;
      // case 'v':
      //   TODO: Enable verbose
      //   break;
    default:
      break;
    }
  }
  auto ptr =
      std::make_unique<Test::TestDwayLazyReset>(config_file);
  ptr->runTest();
  return 0;
}

static void Help(const char *ptr) {
  fmt::print("Usage: {} [-c config]\n", ptr);
  exit(0);
}
