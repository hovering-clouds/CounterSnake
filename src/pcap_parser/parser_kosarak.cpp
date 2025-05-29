/**
 * @file parser.cpp
 * @author hc (you@domain.com)
 * @brief Parser for kosarak dataset
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <IPv4Layer.h>
#include <IcmpLayer.h>
#include <Packet.h>
#include <PcapFileDevice.h>
#include <PcapPlusPlusVersion.h>
#include <RawPacket.h>
#include <SystemUtils.h>
#include <TcpLayer.h>
#include <UdpLayer.h>
#include <common/data.h>
#include <iostream>
#include <fstream>
#include <getopt.h>


template <int32_t key_len>
int32_t dumpTextInBinary(
  OmniSketch::Data::DataFormat *format, 
  std::string input_file, 
  std::string output_file,
  int verbose_level) {
  if (!format) {
    throw std::runtime_error("Runtime Error: No format is specified.");
  }

  std::ifstream fin(input_file);
  if(!fin.is_open()){
    throw std::runtime_error("Runtime Error: Cannot open file " + input_file);
  }

  std::ofstream fout(output_file, std::ios::binary); // automatically destroyed
  if (!fout.is_open()) {
    throw std::runtime_error("Runtime Error: Could not open output file " + output_file);
  }

  // packet count
  size_t packet_count_so_far = 0;
  // flow sets
  OmniSketch::Data::Estimation<key_len, int64_t> all_flows;

  int32_t flow_id;
  while (fin >> flow_id) {

    OmniSketch::FlowKey<key_len> *key_ptr = nullptr;
    if (key_len == 4) {
      key_ptr = new OmniSketch::FlowKey<key_len>(flow_id);
    } else if (key_len == 8) {
      key_ptr = new OmniSketch::FlowKey<key_len>(flow_id, flow_id);
    } else if (key_len == 13) {
      key_ptr = new OmniSketch::FlowKey<key_len>(flow_id, flow_id, 0, 1, 2);
    }
    all_flows.insert(*key_ptr);

    // deserialize flow key
    OmniSketch::Data::Record<key_len> record;
    record.flowkey.copy(0, *key_ptr, 0, key_len);
    record.length = 1;
    record.timestamp = packet_count_so_far;
    int8_t byte[format->getRecordLength()];
    format->writeAsFormat(record, byte);
    delete key_ptr;
    // write to file
    fout.write(reinterpret_cast<const char *>(byte), format->getRecordLength());

    // verbosity: per-packet info
    if (verbose_level > 1) {
      std::cout << "#" << packet_count_so_far << std::endl;
      std::cout << flow_id << std::endl;
    }

    packet_count_so_far += 1;
  }

  // verbosity: file info
  if (verbose_level > 0) {
    std::cout << "Finished. Printed " << packet_count_so_far << " packets ("
              << all_flows.size() << " flows)" << std::endl;
  }

  // return the number of packets that were printed
  return packet_count_so_far;
}

int main(int argc, char *argv[]) {
  int option_index = 0;
  int opt = 0;
  int verbose_level = 0;
  std::string input_file, output_file;

  struct option pcap_parser_options[] = {{"help", no_argument, 0, 'h'},
                                         {"verbose", no_argument, 0, 'v'},
                                         {"input", required_argument, 0, 'i'},
                                         {"output", required_argument, 0, 'o'},
                                         {0, 0, 0, 0}};
  auto printUsage = []() {
    std::cout << std::endl
              << "Usage:" << std::endl
              << "------" << std::endl
              << pcpp::AppName::get() << " [-i input] [-o output] [-v]" << std::endl
              << std::endl
              << "Options:" << std::endl
              << std::endl
              << "    -i input  : Input file path" << std::endl
              << "    -o output : Output file path" << std::endl
              << "    -v        : Verbose level " << std::endl
              << "    -h        : Display this help message and exit"
              << std::endl
              << std::endl;
  };

  while ((opt = getopt_long(argc, argv, "i:o:vh", pcap_parser_options,
                            &option_index)) != -1) {
    switch (opt) {
    case 0:
      break;
    case 'h':
      printUsage();
      exit(0);
      break;
    case 'v':
      verbose_level += 1;
      break;
    case 'i':
      input_file = optarg;
      break;
    case 'o':
      output_file = optarg;
      break;
    default:
      printUsage();
      exit(-1);
    }
  }
  std::cout << "begin1" << std::endl;

  // Truncate Verbose level
  verbose_level = std::min(verbose_level, 2);
  auto cfg = toml::array{toml::array{"flowkey", "padding", "timestamp", "length", "padding"}, toml::array{13, 3, 8, 2, 6}};
  auto format = new OmniSketch::Data::DataFormat(cfg);
  dumpTextInBinary<13>(format, input_file, output_file, verbose_level);
}