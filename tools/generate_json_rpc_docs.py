#!/usr/bin/env python3
"""
Generates the JSON RPC documentation from the source code and writes it into
../docs/JSON-RPC.md.

Usage:
./tools/generate_json_rpc_docs.py

"""

import os
import re

source_files = [
    "src/rpcserver.cpp",
    "src/serverrpc.cpp",
    "src/clientrpc.cpp",
]

repo_root = os.path.join(os.path.dirname(__file__), '..')


class DocumentationItem:
    """
    Represents a documentation item. In the source code they look like this:

        /// @rpc_notification jamulusclient/channelLevelListReceived
        /// @brief Emitted when the channel level list is received.
        /// @param {array} params.channelLevelList - The channel level list.
        ///  Each item corresponds to the respective client retrieved from the
        ///  jamulusclient/clientListReceived notification.
        /// @param {number} params.channelLevelList[*] - The channel level,
        ///  an integer between 0 and 9.
    """

    def __init__(self, name, type_):
        """
        @param name: str
        @param type: str
        """
        self.name = name
        self.type = type_
        self.brief = DocumentationText()
        self.params = []
        self.results = []

    def handle_tag(self, tag_name):
        """
        @param tag_name: str, brief|param|result
        """
        if tag_name == "brief":
            self.current_tag = self.brief
        elif tag_name == "param":
            self.current_tag = DocumentationText()
            self.params.append(self.current_tag)
        elif tag_name == "result":
            self.current_tag = DocumentationText()
            self.results.append(self.current_tag)
        else:
            raise Exception("Unknown tag: " + tag_name)

    def handle_text(self, text):
        """
        @param text: str
        """
        self.current_tag.add_text(text)

    def sort_key(self):
        """
        @return str
        """
        return self.type + ": " + self.name

    def to_markdown(self):
        """
        @return: Markdown-formatted str with name, brief, params and results
        """
        output = []
        output.append("### " + self.name)
        output.append("")
        output.append(str(self.brief))
        output.append("")

        if len(self.params) > 0:
            output.append("Parameters:")
            output.append("")
            output.append(DocumentationTable(self.params).to_markdown())
            output.append("")

        if len(self.results) > 0:
            output.append("Results:")
            output.append("")
            output.append(DocumentationTable(self.results).to_markdown())
            output.append("")

        return "\n".join(output)


class DocumentationText:
    """
    Represents text inside the documentation.
    """

    def __init__(self):
        """Constructor"""
        self.parts = []

    def add_text(self, text):
        """
        @param text: str
        """
        self.parts.append(text)

    def __str__(self):
        """
        @return: Space-delimited list of previously added parts
        """
        return " ".join(self.parts)


class DocumentationTable:
    """
    Represents parameters and results table.
    """

    def __init__(self, tags):
        """
        @param tags: iterable
        """
        self.tags = tags

    def to_markdown(self):
        """
        @return: str containing the Markdown table
        """
        output = []
        output.append("| Name | Type | Description |")
        output.append("| --- | --- | --- |")
        tag_re = re.compile(r"^\{(\w+)\}\s+([\S]+)\s+-\s+(.*)$", re.DOTALL)
        for tag in self.tags:
            text = str(tag)
            # Parse tag in form of "{type} name - description"
            match = tag_re.match(text)
            if match:
                type_ = match.group(1)
                name = match.group(2)
                description = re.sub(r"^\s+", " ", match.group(3)).strip()
                output.append(f"| {name} | {type_} | {description} |")
        return "\n".join(output)


items = []
current_item = None

# Parse the source code
for source_file in source_files:
    with open(os.path.join(repo_root, source_file), "r") as f:
        for line in f.readlines():
            line = line.strip()
            if line.startswith("/// @rpc_notification "):
                current_item = DocumentationItem(
                    line[len("/// @rpc_notification "):], "notification"
                )
                items.append(current_item)
            elif line.startswith("/// @rpc_method "):
                current_item = DocumentationItem(
                    line[len("/// @rpc_method "):], "method"
                )
                items.append(current_item)
            elif line.startswith("/// @brief "):
                current_item.handle_tag("brief")
                current_item.handle_text(line[len("/// @brief "):])
            elif line.startswith("/// @param "):
                current_item.handle_tag("param")
                current_item.handle_text(line[len("/// @param "):])
            elif line.startswith("/// @result "):
                current_item.handle_tag("result")
                current_item.handle_text(line[len("/// @result "):])
            elif line.startswith("///"):
                current_item.handle_text(line[len("///"):])
            elif line == "":
                pass
            else:
                current_item = None

items.sort(key=lambda item: item.name)

preamble = """
# Jamulus JSON-RPC Documentation

<!--
This file is automatically generated from the source code.
Do not edit this file manually.
See `tools/generate_json_rpc_docs.py` for details.
-->

A JSON-RPC interface is available for both Jamulus client and server to allow programmatic access.
To use the JSON-RPC interface, run Jamulus with the `--jsonrpcport <port> --jsonrpcsecretfile /file/with/a/secret.txt` options.
This will start a JSON-RPC server on the specified port on the localhost.

The file referenced by `--jsonrpcsecretfile` must contain a single line with a freely chosen string with at least 16 characters.
It can be generated like this:
```
$ openssl rand -base64 10 > /file/with/a/secret.txt
```

The JSON-RPC server defaults to listening on the local loopback network interface (127.0.0.1).  This can be optionally changed by using the `--jsonrpcbindip <ip address>` command line option. **IPv4 only. IPv6 support has not been tested.**


## Wire protocol

The JSON-RPC server is based on the [JSON-RPC 2.0](https://www.jsonrpc.org/specification) protocol, using [streaming newline-delimited JSON over TCP](https://clue.engineering/2018/introducing-reactphp-ndjson) as the transport. There are three main types of messages being exchanged:

- A **request** from the consumer to Jamulus.
- A **response** from Jamulus to the consumer.
- A **notification** from Jamulus to the consumer.

## Example

After opening a TCP connection to the JSON-RPC server, the connection must be authenticated:

```json
{"id":1,"jsonrpc":"2.0","method":"jamulus/apiAuth","params":{"secret": "...the secret from the file in --jsonrpcsecretfile..."}}
```

Request must be sent as a single line of JSON-encoded data, followed by a newline character. Jamulus will send back a **response** in the same manner:

```json
{"id":1,"jsonrpc":"2.0","result":"ok"}
```
After successful authentication, the following **request** can be sent:

```json
{"id":2,"jsonrpc":"2.0","method":"jamulus/getMode","params":{}}
```

The request must be sent as a single line of JSON-encoded data, followed by a newline character. Jamulus will send back a **response** in the same manner:

```json
{"id":2,"jsonrpc":"2.0","result":{"mode":"client"}}
```

Jamulus will also send **notifications** to the consumer:

```json
{"jsonrpc":"2.0","method":"jamulusclient/chatTextReceived","params":{"text":"<font color=\\"mediumblue\\">(01:23:45 AM) <b>user</b></font> test"}}
```

"""

docs_path = os.path.join(repo_root, 'docs', 'JSON-RPC.md')
with open(docs_path, "w") as f:
    f.write(preamble)

    f.write("## Method reference\n")
    for item in filter(lambda item: item.type == "method", items):
        f.write(item.to_markdown() + "\n\n")

    f.write("## Notification reference\n")
    for item in filter(lambda item: item.type == "notification", items):
        f.write(item.to_markdown() + "\n\n")
