#!/bin/bash

curl --dump-header headers_log.txt --max-time 5 --trace - --trace-time -H 'Connection: close' --write-out "== Info: Status %{http_code}\n== Info: Bytes %{size_download}\n== Info: Duration %{time_total} sec.\n" http://localhost:8080


