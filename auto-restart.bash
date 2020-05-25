#!/bin/bash

## move the VICE window to monitor 2, top-left corner
## I haven't figured out how to make this work reliably, so far.
#xdotool windowmove $(xdotool search --onlyvisible --name x64sc ) 1920 0"

## entr returns exit code 0 when the user presses Ctrl-C; otherwise it returns
## a non-zero exit code
exit_code=1
while [[ $exit_code != 0 ]]; do
    find src | entr -d -r make test;
    exit_code=$?
    done
