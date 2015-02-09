#==================================================================================
# A common setup file  for all makefile in the source tree. This file will be included
# in Makefile(s) as well as be loaded as properti file. Therefore, specified only simple key-value
# pairs in this file. Variable extensions should use ${} istead of $().
#==================================================================================
# GM_TOP will be generated by the top-level makefile
GM_TOP=/home/stefan/projects/gm

#-------------------------------------------------------------
# 0. Java/Hadoop directories, only required for HDFS suport and
# Giraph backend
#-------------------------------------------------------------
# hadoop distribution (for HDFS suupport and Giraph backend)
HADOOP_HOME=/cm/shared/apps/hadoop/current
#jdk (for HDFS support and Giraph backend)
JAVA_HOME=/usr/java/default
# Name of hadoop core jar (For HDFS support and Giraph backend)
HADOOP_CORE_JAR=hadoop-core-1.0.4.jar
# Colon separated list of jar files, required for HDFS reader
HADOOP_JAR_COLON_LIST=commons-configuration-1.6.jar:commons-logging-1.1.1.jar:commons-lang-2.4.jar


#-------------------------------------------------------------
# 1. Building the compiler
#-------------------------------------------------------------
# (nothing)
CODE_COVERAGE=0


#-------------------------------------------------------------
# 2. Compiling the GM applications
#-------------------------------------------------------------
# Choose the backend where the gm source file is be compiled into:
#  cpp_omp, cpp_seq, gps, giraph
env=cpp_omp

# gm_comp arguments to be applied when compiling sample programs
GM_ARGS=

# Pacakge name of generated giraph applications (for Giraph)
OUTPUT_PACKAGE=my.app

#-------------------------------------------------------------
# 3. Building shared-memory backend (gm_graph)
#-------------------------------------------------------------
# Flag for Oracle SPARC environment
ORACLE=0

# Enforce 64bit binary or 32bit binary
FORCE_64BIT=0
FORCE_32BIT=0

# Bit width of node-id type and, edge-id type:
# one of (32,32), (32,64), (64,64)
NODE_SIZE=32
EDGE_SIZE=32

# Support reading/writing graph files from HDFS
SUPPORT_HDFS=0

# Directories for SUPPORT_HDFS
#location of libhdfs.so
LIBHDFS_DIR=${HADOOP_HOME}/c++/Linux-amd64-64/lib
#location of hdfs.h
INCHDFS_DIR=${HADOOP_HOME}/src/c++/libhdfs
#location of libhdfs.so
LIBJVM_DIR=${JAVA_HOME}/jre/lib/amd64/server
#location of jni.h
INCJNI_DIR=${JAVA_HOME}/include
#location of jni_md.h
INCJNIMD_DIR=${JAVA_HOME}/include/linux
#location of hadoop core
HADOOP_CORE_DIR=${HADOOP_HOME}
#HADOOP_CORE_DIR=${GM_TOP}/apps/output_giraph/lib

# Support reading/writing adjacency list in Avro format
SUPPORT_AVRO=0

#--------------------------------------------------------
# 4. Building Giraph bakcend
#--------------------------------------------------------
# Create jar for exmple giraph apps: Need HADOOP_HOME, HADOOP_CORE_JAR >= 1.0.3
CREATE_JAR_FOR_EXAMPLE_GIRAPH_APPS=0
