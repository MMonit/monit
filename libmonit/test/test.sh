#!/bin/sh

PATH="$PATH:."
export PATH

StrTest && \
IntTest && \
FmtTest && \
TimeTest && \
SystemTest && \
RandomTest && \
ArrayTest && \
ListTest && \
StringBufferTest && \
DirTest && \
InputStreamTest && \
OutputStreamTest && \
FileTest && \
ExceptionTest && \
NetTest && \
CommandTest
