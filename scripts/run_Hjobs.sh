#!/bin/bash

CONFIG=config_histo.ini
MIN_EVENT=0
MAX_EVENT=16755464

JOB_NAME=TTjobH
ROOT_OUTPUT=TToutH

SCRIPT_DIR=job_Hscripts
OUTPUT_DIR=resultsH

JOBS=101
MIN_JOB=1
MAX_JOB=100

python generate_Hjobs.py -j ${JOBS} --min-event ${MIN_EVENT} --max-event ${MAX_EVENT} --job-name ${JOB_NAME} --output ${ROOT_OUTPUT} --dir ${SCRIPT_DIR} --output-dir ${OUTPUT_DIR} --config-file ${CONFIG} -v
./add_jobs.sh -p ${SCRIPT_DIR}/${JOB_NAME}_ -b ${MIN_JOB} -e ${MAX_JOB}
