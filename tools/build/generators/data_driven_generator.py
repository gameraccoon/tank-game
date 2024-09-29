#!/usr/bin/python3

from generators.generator_description_json_updater import update_generator_description_json
from generators.shared_functions import *

delimiter_dictionary_file = "delimiter_dictionary.json"
files_to_generate_file = "files_to_generate.json"
generator_description_file = "generator_description.json"


def generate_all(generator):
    configs_dir = generator["configs_dir"]
    templates_dir = path.join(configs_dir, generator["templates_dir"])
    descriptions_dir = generator["descriptions_dir"]
    output_dir_base = generator["output_dir_base"]

    generator_name = generator["name"]

    if not path.exists(configs_dir):
        print_generator_error(generator_name, "Configs directory '{}' does not exist".format(configs_dir))
        exit(1)

    if not path.exists(path.join(configs_dir, generator_description_file)):
        print_generator_error(generator_name, "Generator description file '{}' does not exist".format(generator_description_file))
        exit(1)
    generator_description = load_json(path.join(configs_dir, generator_description_file))
    generator_description, _ = update_generator_description_json(generator_description)

    if not path.exists(output_dir_base):
        print_generator_error(generator_name, "Output directory '{}' does not exist".format(output_dir_base))
        exit(1)

    if not path.exists(templates_dir):
        print_generator_error(generator_name, "Templates directory '{}' does not exist".format(templates_dir))
        exit(1)

    if not path.exists(path.join(configs_dir, delimiter_dictionary_file)):
        print_generator_error(generator_name, "Delimiter dictionary file '{}' does not exist".format(delimiter_dictionary_file))
        exit(1)
    delimiter_dictionary = load_json(path.join(configs_dir, delimiter_dictionary_file))
    empty_delimiter_dictionary = {key: "" for key in delimiter_dictionary.keys()}

    if not path.exists(path.join(configs_dir, files_to_generate_file)):
        print_generator_error(generator_name, "Files to generate file '{}' does not exist".format(files_to_generate_file))
        exit(1)
    files_to_generate = load_json(path.join(configs_dir, files_to_generate_file))

    if descriptions_dir or "description_schema" in generator_description:
        if descriptions_dir and "description_schema" in generator_description:
            if not path.exists(descriptions_dir):
                print_generator_error(generator_name, "Descriptions directory '{}' does not exist".format(descriptions_dir))
                exit(1)
        elif descriptions_dir:
            print_generator_error(generator_name, "Descriptions directory is provided, but description schema is not used")
            exit(1)
        else:
            print_generator_error(generator_name, "Description schema is used, but descriptions directory is not provided")
            exit(1)

    # precompute some values: e.g. load files by paths
    preprocess_generator_description(generator_description, configs_dir, generator_name)

    generated_files = []
    # descriptions = []
    # raw_descriptions = []
    # for file_name in os.listdir(descriptions_dir):
    #     description = load_data_description(path.join(descriptions_dir, file_name), attribute_optional_fields)
    #     description["component_name"] = get_component_name_from_file_name(file_name)
    #     full_data_dict = get_full_data_dictionary(description, attribute_templates, templates_dir, delimiter_dictionary, empty_delimiter_dictionary)
    #     generated_files += generate_files(files_to_generate, description, full_data_dict, output_dir_base, templates_dir)
    #     description["data_dict"] = full_data_dict
    #     raw_descriptions.append(description)
    #     descriptions.append(full_data_dict)
    #
    # # generate component lists
    # component_filled_templates = generate_component_list_descriptions(descriptions, component_templates, templates_dir, delimiter_dictionary, empty_delimiter_dictionary)
    # for file_info in files_to_generate:
    #     if "flags" in file_info and "list" in file_info["flags"]:
    #         out_file_path = generate_cpp_file(file_info["template"],
    #             path.join(output_dir_base, file_info["output_dir"]),
    #             file_info["name_template"],
    #             component_filled_templates, templates_dir)
    #         generated_files.append(out_file_path)
    #
    # # generate attributes lists
    # attribute_filled_templates = generate_attribute_list_descriptions(raw_descriptions, attribute_templates, templates_dir, delimiter_dictionary, empty_delimiter_dictionary)
    # for file_info in files_to_generate:
    #     if "flags" in file_info and "attribute_list" in file_info["flags"]:
    #         out_file_path = generate_cpp_file(file_info["template"],
    #             path.join(output_dir_base, file_info["output_dir"]),
    #             file_info["name_template"],
    #             attribute_filled_templates, templates_dir)
    #         generated_files.append(out_file_path)

    return generated_files


def print_generator_error(name, message):
    print("Error in generator '{}': {}".format(name, message))


def preprocess_generator_description(generator_description, configs_dir, generator_name):
    description_schema = generator_description.get("description_schema")
    if description_schema:
        preprocess_generator_description_recursively(description_schema, configs_dir, generator_name)

    # special cases for "root_description_object" object in the root
    root_description_object = generator_description.get("root_object")
    if root_description_object:
        templates_file = root_description_object.get("templates_file")
        if templates_file:
            # ToDo: validate if the file exists
            root_description_object["templates"] = load_json(path.join(configs_dir, templates_file))
            root_description_object["templates_file"] = None

    # special cases for "file_name" object in the root
    file_name = generator_description.get("file_name")
    if file_name:
        derived_placeholders = file_name.get("derived_placeholders")
        if derived_placeholders:
            generate_modifier_functions_for_list(derived_placeholders, generator_name)


def preprocess_generator_description_recursively(json_object, configs_dir, generator_name):
    for value in json_object.values():
        templates_file = value.get("templates_file")
        if templates_file:
            # ToDo: validate if the file exists
            value["templates"] = load_json(path.join(configs_dir, templates_file))
            value["templates_file"] = None

        derived_placeholders = value.get("derived_placeholders")
        if derived_placeholders:
            generate_modifier_functions_for_list(derived_placeholders, generator_name)

        list_item_schema = value.get("list_item_schema")
        if list_item_schema:
            # ToDo: validate if that was an object
            preprocess_generator_description_recursively(list_item_schema, configs_dir, generator_name)


def generate_modifier_functions_for_list(placeholder, generator_name):
    for key, value in placeholder.items():
        if isinstance(value, list):
            modifier_function = lambda x: x

            for modifier in value:
                if not modifier.startswith("modifier:"):
                    print_generator_error(generator_name, "Modifier '{}' does not start with 'modifier:'".format(modifier))
                    exit(1)

                if modifier == "modifier:to_lower":
                    modifier_function = append_modifier_function(modifier_function, lambda x: x.lower())
                elif modifier == "modifier:to_upper":
                    modifier_function = append_modifier_function(modifier_function, lambda x: x.upper())
                elif modifier == "modifier:lower_first":
                    modifier_function = append_modifier_function(modifier_function, lambda x: x[:1].lower() + x[1:])
                elif modifier.startswith("modifier:remove_suffix"):
                    suffix_start = modifier.find("(")
                    suffix_end = modifier.find(")")
                    suffix = modifier[suffix_start + 1:suffix_end]
                    modifier_function = append_modifier_function(modifier_function, lambda x: x[:-len(suffix)] if x.endswith(suffix) else x)
                else:
                    print_generator_error(generator_name, "Unknown modifier '{}'".format(modifier))
                    exit(1)

            placeholder[key] = modifier_function


def append_modifier_function(left_function, right_function):
    return lambda x: right_function(left_function(x))

