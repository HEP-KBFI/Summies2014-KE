#!/bin/bash

CONFIG=config_sample.ini
MIN_EVENT=0
MAX_EVENT=16755464

JOB_NAME=TTjobS
ROOT_OUTPUT=TToutS

SCRIPT_DIR=job_Sscripts
OUTPUT_DIR=resultsS

HISTOGRAMS=TTresults.root

JOBS=101
MIN_JOB=1
MAX_JOB=100

python generate_Sjobs.py -j ${JOBS} --min-event ${MIN_EVENT} --max-event ${MAX_EVENT} --job-name ${JOB_NAME} --output ${ROOT_OUTPUT} --dir ${SCRIPT_DIR} --output-dir ${OUTPUT_DIR} --config-file ${CONFIG} -v --histograms ${HISTOGRAMS}
./add_jobs.sh -p ${SCRIPT_DIR}/${JOB_NAME}_ -b ${MIN_JOB} -e ${MAX_JOB}
