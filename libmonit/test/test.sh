#!/bin/sh

PATH="$PATH:."
export PATH

StrTest && \
NumTest && \
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
