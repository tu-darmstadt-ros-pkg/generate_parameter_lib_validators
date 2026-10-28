
# generate_parameter_lib_validators

This package provides reusable validation hooks that can be referenced directly from parameter YAML files and are applied automatically by the generated `ParamListener`.

---

## General usage

### Add dependency

**package.xml**

```xml
<depend>generate_parameter_lib_validators</depend>
<depend>generate_parameter_library</depend>
```

### CMake Integration

The package provides a CMake helper function `generate_parameter_library_with_validators` which wraps the standard generator to automatically include the validator headers and link the necessary dependencies.

```cmake
find_package(generate_parameter_library REQUIRED)
find_package(generate_parameter_lib_validators REQUIRED)

# Generate the library using the helper function
generate_parameter_library_with_validators(
  my_parameters
  config/my_parameters.yaml
)

# Link your main target against the generated parameter library
# (The validators are linked automatically to 'my_parameters')
target_link_libraries(my_node my_parameters)

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

### `valid_relative_topic_name`

Same rules and supported types as `valid_topic_name`, but additionally rejects absolute (`/foo`) and private (`~`, `~/foo`) names.
Use it for names that get appended to another namespace (e.g. `"~/" + name`).
