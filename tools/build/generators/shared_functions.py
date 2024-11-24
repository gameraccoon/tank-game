#!/usr/bin/python3

import json
import os
from os import path


def capitalize(input_str):
    return input_str[:1].capitalize() + input_str[1:]


def read_template(template_name, templates_dir):
    with open(path.join(templates_dir, template_name + ".template"), 'r') as template_file:
        return template_file.read().rstrip('\n')


def replace_content(template_string, data_dictionary):
    return template_string.format(**data_dictionary)


def write_file(generated_file_name, content):
    if os.path.isfile(generated_file_name):
        with open(generated_file_name, 'r') as generated_file:
            if content + '\n' == generated_file.read():
                return

    with open(generated_file_name, 'w') as generated_file:
        generated_file.write(content + '\n')


def load_json(file_path):
    with open(file_path) as f:
        return json.load(f)


def generate_cpp_file(template_name, destination_dir, file_name_template, filled_templates, templates_dir):
    template = read_template(template_name, templates_dir)
    generated_content = replace_content(template, filled_templates)
    file_name = replace_content(file_name_template, filled_templates)

    if not os.path.exists(destination_dir):
        os.makedirs(destination_dir)

    out_file_path = path.join(destination_dir, file_name)
    write_file(out_file_path, generated_content)
    return out_file_path
