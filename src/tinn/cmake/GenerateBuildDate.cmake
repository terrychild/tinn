string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")
configure_file("${TEMPLATE_FILE}" "${OUTPUT_FILE}" @ONLY)