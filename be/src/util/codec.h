// Copyright (c) 2012 Cloudera, Inc. All rights reserved.

#ifndef IMPALA_EXEC_CODEC_H
#define IMPALA_EXEC_CODEC_H

#include "exec/hdfs-scanner.h"
#include "runtime/mem-pool.h"
#include "gen-cpp/Descriptors_types.h"

namespace impala {

// Create a compression object.  This is the base class for all compression
// algorithms. A compression algorithm is either a compressor or a decompressor.
// To add a new algorithm, generally, both a compressor and a decompressor
// will be added.  Each of these objects inherits from this class. The objects
// are instantiated in the Create static methods defined here.  The type of
// compression is defined in the Thrift interface THdfsCompression.
class Codec {
 public:
  // These are the codec string representation used in Hadoop.
  static const char* const DEFAULT_COMPRESSION;
  static const char* const GZIP_COMPRESSION;
  static const char* const BZIP2_COMPRESSION;
  static const char* const SNAPPY_COMPRESSION;

  // Map from codec string to compression format
  static const std::map<const std::string, const THdfsCompression> CODEC_MAP;

  // Create a decompressor.
  // Input: 
  //  runtime_state: the current runtime state.
  //  mem_pool: the memory pool used to store the decompressed data.
  //  reuse: if true the allocated buffer can be reused.
  //  format: the type of decompressor to create.
  // Output:
  //  decompressor: pointer to the decompressor class to use.
  static Status CreateDecompressor(RuntimeState* runtime_state, MemPool* mem_pool,
                                   bool reuse, THdfsCompression::type format,
                                   Codec** decompressor);

  // Alternate creator: takes a codec string and returns a scoped pointer.
  static Status CreateDecompressor(RuntimeState* runtime_state, MemPool* mem_pool,
                                   bool reuse, const std::vector<char>& codec,
                                   boost::scoped_ptr<Codec>* decompressor);

  // Create the compressor.
  // Input: 
  //  runtime_state: the current runtime state.
  //  mem_pool: the memory pool used to store the compressed data.
  //  format: The type of compressor to create.
  //  reuse: if true the allocated buffer can be reused.
  // Output:
  //  compressor: pointer to the compressor class to use.
  static Status CreateCompressor(RuntimeState* runtime_state, MemPool* mem_pool,
                                 bool reuse, THdfsCompression::type format,
                                 Codec** decompressor);

  // Alternate creator: takes a codec string and returns a scoped pointer.
  // Input, as above except:
  //  codec: the string representing the codec of the current file.
  static Status CreateCompressor(RuntimeState* runtime_state, MemPool* mem_pool,
                                 bool reuse, const std::vector<char>& codec,
                                 boost::scoped_ptr<Codec>* compressor);

  virtual ~Codec() {}

  // Process a block of data.  The operator will allocate the output buffer
  // if output_length is passed as 0 and return the length in output_length.
  // If it is non-zero the length must be the correct size to hold the transformed output.
  // Inputs:
  //   input_length: length of the data to process
  //   input: data to process
  // In/Out:
  //   output_length: Length of the output, if known, 0 otherwise.
  // Output:
  //   output: Pointer to processed data
  virtual Status ProcessBlock(int input_length, uint8_t* input,
                              int* output_length, uint8_t** output)  = 0;

  // Return the name of a compression algorithm.
  static std::string GetCodecName(THdfsCompression::type);

 protected:
  // Create a compression operator
  // Inputs:
  //   mem_pool: memory pool to allocate the output buffer, this implies that the
  //             caller is responsible for the memory allocated by the operator.
  //   reuse_buffer: if false always allocate a new buffer rather than reuse.
  Codec(MemPool* mem_pool, bool reuse_buffer);

  // Initialize the operation.
  virtual Status Init() = 0;

  // Pool to allocate the buffer to hold transformed data.
  MemPool* memory_pool_;

  // Temporary memory pool: in case we get the output size too small we can
  // use this to free unused buffers.
  MemPool temp_memory_pool_;

  // Can we reuse the output buffer or do we need to allocate on each call?
  bool reuse_buffer_;

  // Buffer to hold transformed data.
  // Either passed from the caller or allocated from memory_pool_.
  uint8_t* out_buffer_;

  // Length of the output buffer.
  int buffer_length_;
};

}
#endif
