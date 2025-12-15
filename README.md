# generate_parameter_lib_validators

git
The package provides reusable validation hooks that can be referenced directly from parameter YAML files and are applied automatically by the generated `ParamListener`.

---

## General usage

### Add dependency

**package.xml**

```xml
<depend>generate_parameter_lib_validators</depend>
<depend>generate_parameter_library</depend>
```

### CMake Integration

Expose the validator header to the parameter code generator:

```cmake
find_package(generate_parameter_library REQUIRED)
find_package(generate_parameter_lib_validators REQUIRED)

set(GPLV_TARGET generate_parameter_lib_validators::generate_parameter_lib_validators)

get_target_property(GPLV_INCLUDE_DIRS ${GPLV_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
list(GET GPLV_INCLUDE_DIRS 0 GPLV_INCLUDE_DIR)

generate_parameter_library(
  my_parameters
  config/my_parameters.yaml
  "${GPLV_INCLUDE_DIR}/generate_parameter_lib_validators/parameter_validators.hpp"
)

target_link_libraries(my_parameters INTERFACE ${GPLV_TARGET})
```

### YAML usage

```yaml
my_node:
  output_topic:
    type: string
    validation:
      "generate_parameter_lib_validators::valid_topic_name": []

  input_topics:
    type: string_array
    validation:
      "generate_parameter_lib_validators::valid_topic_name": []
```

The same validator works for both `string` and `string_array`.

---

## Validators

### `valid_topic_name`

Validates ROS 2 topic names according to DDS-compatible rules.

**Supported parameter types**

* `string`
* `string_array` (each entry is validated)

**Rules enforced**

* Segments separated by `/`
* Segment regex:
  `[A-Za-z_][A-Za-z0-9_]*`
* No empty segments, no trailing `/`
* Absolute topics allowed: `/foo/bar`
* Private topics allowed:

    * `~`
    * `~/foo/bar`
* `~` may only appear as the first character
* Allowed characters: `A-Z a-z 0-9 _ /` (plus optional leading `~`)
* Maximum length: **256 characters**

**Failure behavior**

* Returns a descriptive error via `tl::expected`
* For string arrays, reports the failing index
