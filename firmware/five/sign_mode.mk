ifndef SEC_BUILD_CONF_SIGNER_MODEL_NAME
  $(error "SEC_BUILD_CONF_SIGNER_MODEL_NAME isn't defined")
endif
five_sign_runtype := qc_secimg50_tzapp
five_sign_model := $(SEC_BUILD_CONF_SIGNER_MODEL_NAME)
