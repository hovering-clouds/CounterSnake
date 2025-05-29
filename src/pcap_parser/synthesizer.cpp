/**
 * @file synthesizer.cpp
 * @author dromniscience (you@domain.com)
 * @brief
 * @date 2022-08-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "synthesizer.h"
#include <getopt.h>

int main(int argc, char *argv[]) {
  
  int option_index = 0;
  int opt = 0;
  
  double skew = 2.0;
  int32_t maximum = 20000;
  int32_t flow_number = 100000;
  std::string config_file = "../src/pcap_parser/parser.toml";
  std::string work_node = "synthesizer";
  std::string output_file = "../data/test.bin";

  struct option pcap_parser_options[] = {{"help", no_argument, 0, 'h'},
                                         {"config", required_argument, 0, 'c'},
                                         {"node", required_argument, 0, 'n'},
                                         {0, 0, 0, 0}};
  auto printUsage = []() {
    std::cout << std::endl
              << "Options:" << std::endl
              << std::endl
              << "    -c config   : Config file for synthesizer" << std::endl
              << "    -n node     : Working node in config file" << std::endl
              << std::endl;
  };

  while ((opt = getopt_long(argc, argv, "cn:h", pcap_parser_options,
                            &option_index)) != -1) {
    switch (opt) {
    case 0:
      break;
    case 'h':
      printUsage();
      exit(0);
      break;
    case 'c':
      config_file = optarg;
      break;
    case 'n':
      work_node = optarg;
      break;
    default:
      printUsage();
      exit(-1);
    }
  }

  OmniSketch::Util::ConfigParser parser(config_file);
  parser.setWorkingNode(work_node);
  toml::array arr;
  if (!parser.parseConfig(arr, "format")) {
    return 1;
  }
  parser.parseConfig(skew, "skew", false);
  parser.parseConfig(maximum, "maximum", false);
  parser.parseConfig(flow_number, "flow_number", false);
  parser.parseConfig(output_file, "output_file", false);

  OmniSketch::Data::DataFormat fmt(arr);
  OmniSketch::Util::ZipfSynthesizer<13> generator;
  generator.Generate(skew, maximum, flow_number, fmt, output_file);
}
