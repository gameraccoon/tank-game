#!/usr/bin/python3

from generators.generator_configuration_json_updater import update_generator_configuration_json
from generators.shared_functions import *

delimiter_dictionary_file = "delimiter_dictionary.json"
files_to_generate_file = "files_to_generate.json"
generator_configuration_file = "generator_configuration.json"


def generate_all(generator):
    configs_dir = generator["configs_dir"]
    templates_dir = path.join(configs_dir, generator["templates_dir"])
    descriptions_dir = generator["descriptions_dir"]
    output_dir_base = generator["output_dir_base"]

    generator_name = generator["name"]

    if not path.exists(configs_dir):
        generator_error(generator_name, "Configs directory '{}' does not exist".format(configs_dir))

    if not path.exists(path.join(configs_dir, generator_configuration_file)):
        generator_error(generator_name, "Generator description file '{}' does not exist".format(generator_configuration_file))
    generator_configuration = load_json(path.join(configs_dir, generator_configuration_file))
    generator_configuration, _ = update_generator_configuration_json(generator_configuration)

    if not path.exists(output_dir_base):
        generator_error(generator_name, "Output directory '{}' does not exist".format(output_dir_base))

    if not path.exists(templates_dir):
        generator_error(generator_name, "Templates directory '{}' does not exist".format(templates_dir))

    if not path.exists(path.join(configs_dir, delimiter_dictionary_file)):
        generator_error(generator_name, "Delimiter dictionary file '{}' does not exist".format(delimiter_dictionary_file))
    delimiter_dictionary = load_json(path.join(configs_dir, delimiter_dictionary_file))
    empty_delimiter_dictionary = {key: "" for key in delimiter_dictionary.keys()}

    if not path.exists(path.join(configs_dir, files_to_generate_file)):
        generator_error(generator_name, "Files to generate file '{}' does not exist".format(files_to_generate_file))
    files_to_generate = load_json(path.join(configs_dir, files_to_generate_file))

    if descriptions_dir or "description_schema" in generator_configuration:
        if descriptions_dir and "description_schema" in generator_configuration:
            if not path.exists(descriptions_dir):
                generator_error(generator_name, "Descriptions directory '{}' does not exist".format(descriptions_dir))
        elif descriptions_dir:
            generator_error(generator_name, "Descriptions directory is provided, but description schema is not used")
        else:
            generator_error(generator_name, "Description schema is used, but descriptions directory is not provided")

    # precompute some values: e.g. load files by paths
    preprocess_generator_configuration(generator_configuration, configs_dir, generator_name)

    # group files to generate into categories
    files_to_generate_map = group_files_to_generate(files_to_generate)

    generated_files = []

    # recursively iterate over generator configuration and fill dictionaries using descriptions
    if descriptions_dir:
        for file_name in os.listdir(descriptions_dir):
            description = load_json(path.join(descriptions_dir, file_name))
            placeholder_dictionary = {}
            # file_name can have some placeholders that are derived from the name of the description file
            append_file_name_placeholders(placeholder_dictionary, generator_configuration, file_name.rstrip(".json"))
            # top level placeholders are likely to be used down the line, so we always keep those
            description_schema = generator_configuration.get("description_schema")
            if description_schema:
                append_one_level_dictionary(placeholder_dictionary, description_schema, description, generator_name)
                print("Append one level dict: {}".format(placeholder_dictionary))


    # if descriptions_dir:
    #     for file_name in os.listdir(descriptions_dir):
            # generate per-description files
            if "per_object" in files_to_generate_map:
                root_object_id = None
                if "root_description_object" in generator_configuration:
                    root_object_id = generator_configuration["root_description_object"].get("object_id")

                per_root_object = files_to_generate_map["per_object"][root_object_id]
                if per_root_object:
                    for file_description in per_root_object:
                        print("Dictionary: {}".format(placeholder_dictionary))
                        file_name = replace_content(file_description.get("name_template"), placeholder_dictionary)
                        output_path = os.path.join(file_description.get("output_dir"), file_name)
                        print("Generating file: {}".format(output_path))
                        print("Generating file: {}".format(file_description))
                        generated_files.append(generate_cpp_file(file_description["template"], output_dir_base, file_description["name_template"], placeholder_dictionary, templates_dir))

                # recursively generate files based on the description schema

        # generate list files
        list_objects = files_to_generate_map.get("list_objects")
        if list_objects:
            pass

    return generated_files


def generator_error(name, message):
    print("Error in generator '{}': {}".format(name, message))
    exit(1)


def preprocess_generator_configuration(generator_configuration, configs_dir, generator_name):
    description_schema = generator_configuration.get("description_schema")
    if description_schema:
        preprocess_generator_configuration_recursively(description_schema, configs_dir, generator_name)

    # special cases for "root_description_object" object in the root
    root_description_object = generator_configuration.get("root_object")
    if root_description_object:
        try_load_template(root_description_object, configs_dir, generator_name)

    # special cases for "file_name" object in the root
    file_name = generator_configuration.get("file_name")
    if file_name:
        derived_placeholders = file_name.get("derived_placeholders")
        if derived_placeholders:
            generate_modifier_functions_for_list(derived_placeholders, generator_name)


def preprocess_generator_configuration_recursively(json_object, configs_dir, generator_name):
    for value in json_object.values():
        try_load_template(value, configs_dir, generator_name)

        derived_placeholders = value.get("derived_placeholders")
        if derived_placeholders:
            generate_modifier_functions_for_list(derived_placeholders, generator_name)

        list_item_schema = value.get("list_item_schema")
        if list_item_schema:
            if not isinstance(list_item_schema, dict):
                generator_error(generator_name, "List item schema is not an object")
            preprocess_generator_configuration_recursively(list_item_schema, configs_dir, generator_name)


def try_load_template(value, configs_dir, generator_name):
    templates_file = value.get("templates_file")
    if templates_file:
        if not path.exists(path.join(configs_dir, templates_file)):
            generator_error(generator_name, "Templates file '{}' does not exist".format(templates_file))
        value["templates"] = load_json(path.join(configs_dir, templates_file))
        value["templates_file"] = None


def generate_modifier_functions_for_list(placeholder, generator_name):
    available_modifiers = [
        # one argument
        {
            "modifier:to_lower": lambda x: x.lower(),
            "modifier:to_upper": lambda x: x.upper(),
            "modifier:lower_first": lambda x: x[:1].lower() + x[1:],
        },
        # two arguments
        {
            "modifier:remove_suffix": lambda args: lambda x: x[:-len(args[0])] if x.endswith(args[0]) else x,
        }
    ]

    for key, value in placeholder.items():
        if isinstance(value, list):
            modifier_function = lambda x: x

            for modifier in value:
                if not modifier.startswith("modifier:"):
                    generator_error(generator_name, "Modifier '{}' does not start with 'modifier:'".format(modifier))

                if not modifier.endswith(")"):
                    # singe argument modifier
                    # find modifier in available_modifiers[0]
                    new_modifier_function = available_modifiers[0].get(modifier)
                    if not new_modifier_function:
                        generator_error(generator_name, "Unknown modifier '{}'".format(modifier))
                    modifier_function = append_modifier_function(modifier_function, new_modifier_function)
                else:
                    # count number of arguments
                    arguments_start = modifier.find("(")
                    arguments_end = len(modifier) - 1
                    arguments = modifier[arguments_start + 1:arguments_end]
                    arguments = arguments.split(",")
                    arguments = [arg.strip() for arg in arguments]

                    modifier_name = modifier[:arguments_start]

                    len_arguments = len(arguments)
                    if len_arguments >= len(available_modifiers):
                        generator_error(generator_name, "Modifier '{}' has too many arguments".format(modifier))

                    new_modifier_function = available_modifiers[len_arguments].get(modifier_name)
                    if not new_modifier_function:
                        generator_error(generator_name, "Unknown modifier '{}' taking {} arguments".format(modifier_name, len_arguments))

                    modifier_function = append_modifier_function(modifier_function, new_modifier_function(arguments))

            placeholder[key] = modifier_function


def append_modifier_function(left_function, right_function):
    return lambda x: right_function(left_function(x))


def append_file_name_placeholders(result_dictionary, generator_configuration, file_name):
    file_name_obj = generator_configuration.get("file_name")
    if file_name:
        derived_placeholders = file_name_obj.get("derived_placeholders")
        if derived_placeholders:
            for key, value in derived_placeholders.items():
                result_dictionary[key] = value(file_name)


def append_one_level_dictionary(result_dictionary, json_object, description, generator_name):
    # iterate over fields that have "placeholder" key
    for key, value in json_object.items():
        placeholder_name = value.get("placeholder")
        if placeholder_name:
            key_description_value = description.get(key)
            if key_description_value is None:
                is_optional = value.get("is_optional")
                if is_optional is False or is_optional is None:
                    generator_error(generator_name, "Placeholder '{}' is not optional and not found in description".format(key))
                else:
                    continue

            placeholder_type = value.get("type")
            if placeholder_type == "string":
                if not isinstance(key_description_value, str):
                    generator_error(generator_name, "Placeholder '{}' is not a string".format(key))
                result_dictionary[placeholder_name] = key_description_value

            elif placeholder_type == "list":
                if not isinstance(key_description_value, list):
                    generator_error(generator_name, "Placeholder '{}' is not a list".format(key))

                list_placeholder_rule = value.get("list_placeholder_rule")
                if list_placeholder_rule:
                    if list_placeholder_rule == "merge_with_newline":
                        result_dictionary[placeholder_name] = "\n".join(key_description_value)
                    elif list_placeholder_rule == "meta_attributes":
                        result_dictionary[placeholder_name] = key_description_value
                    else:
                        generator_error(generator_name, "Unknown list placeholder rule '{}'".format(list_placeholder_rule))
                else:
                    generator_error(generator_name, "List placeholder '{}' does not have list_placeholder_rule".format(key))

            derived_placeholders = value.get("derived_placeholders")
            if derived_placeholders:
                for derived_key, derived_value in derived_placeholders.items():
                    result_dictionary[derived_key] = derived_value(key_description_value)


def group_files_to_generate(files_to_generate):
    files_to_generate_map = {}

    for file_description in files_to_generate:
        per_object = file_description.get("per_object")
        if per_object:
            if "per_object" not in files_to_generate_map:
                files_to_generate_map["per_object"] = {}
            if per_object not in files_to_generate_map["per_object"]:
                files_to_generate_map["per_object"][per_object] = []
            files_to_generate_map["per_object"][per_object].append(file_description)

        list_objects = file_description.get("list_objects")
        if list_objects:
            if "list_objects" not in files_to_generate_map:
                files_to_generate_map["list_objects"] = {}
            for list_object in list_objects:
                if list_object not in files_to_generate_map["list_objects"]:
                    files_to_generate_map["list_objects"][list_object] = []
                files_to_generate_map["list_objects"][list_object].append(file_description)

    return files_to_generate_map


def generate_cpp_file(template_name, destination_dir, file_name_template, filled_templates, templates_dir):
    template = read_template(template_name, templates_dir)
    generated_content = replace_content(template, filled_templates)
    file_name = replace_content(file_name_template, filled_templates)

    if not os.path.exists(destination_dir):
        os.makedirs(destination_dir)

    out_file_path = path.join(destination_dir, file_name)
    write_file(out_file_path, generated_content)
    print("Generated file: {}".format(out_file_path))
    return out_file_path
