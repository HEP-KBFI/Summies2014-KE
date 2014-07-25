#!/bin/bash

CONFIG=config.ini
MIN_EVENT=0
MAX_EVENT=16755464

JOB_NAME=TTjob
ROOT_OUTPUT=TTout

SCRIPT_DIR=job_scripts
OUTPUT_DIR=results

JOBS=101
MIN_JOB=1
MAX_JOB=10

python generate_jobs.py -j ${JOBS} --min-event ${MIN_EVENT} --max-event ${MAX_EVENT} --job-name ${JOB_NAME} --output ${ROOT_OUTPUT} --dir ${SCRIPT_DIR} --output-dir ${OUTPUT_DIR} --config-file ${CONFIG} -v
./add_jobs.sh -p ${SCRIPT_DIR}/${JOB_NAME}_ -b ${MIN_JOB} -e ${MAX_JOB}
