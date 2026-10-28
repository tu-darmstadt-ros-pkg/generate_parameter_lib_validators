# Included automatically after find_package(generate_parameter_lib_validators)

# Derive this package prefix from this file location:
get_filename_component(_gplv_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.."
                       ABSOLUTE)

set(generate_parameter_lib_validators_VALIDATOR_HEADER
    "${_gplv_prefix}/include/generate_parameter_lib_validators/generate_parameter_lib_validators.hpp"
)

function(generate_parameter_library_with_validators out_target yaml_file)
  # Sanity check to ensure header exists
  if(NOT EXISTS "${generate_parameter_lib_validators_VALIDATOR_HEADER}")
    message(
      FATAL_ERROR
        "Validator header not found: ${generate_parameter_lib_validators_VALIDATOR_HEADER}"
    )
  endif()

  # Sanity check to ensure upstream library is loaded
  if(NOT COMMAND generate_parameter_library)
    message(
      FATAL_ERROR
        "generate_parameter_library function missing. Did you find_package(generate_parameter_library)?"
    )
  endif()

  generate_parameter_library(
    ${out_target} ${yaml_file}
    ${generate_parameter_lib_validators_VALIDATOR_HEADER})

  # Automatically link the validator interface library to the generated target
  target_link_libraries(
    ${out_target}
    INTERFACE
      generate_parameter_lib_validators::generate_parameter_lib_validators)
endfunction()
