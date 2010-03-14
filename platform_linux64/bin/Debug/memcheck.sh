#!/bin/sh
valgrind --tool=memcheck --leak-check=full --log-file=memcheck.log ./vineo