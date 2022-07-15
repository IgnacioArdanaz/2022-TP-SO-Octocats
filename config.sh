#!/bin/bash

echo "Configurando configs para: $1"

cp "./tests/$1/memoria.config" "./memoria/"

cp "./tests/$1/kernel.config" "./kernel/"

cp "./tests/$1/cpu.config" "./cpu/"

