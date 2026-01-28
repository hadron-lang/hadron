#include <iostream>
#include <map>
#include <string>
#include <vector>

// Assuming these headers exist in your project
#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"
#include "frontend/semantic.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace hadron;

// Store open documents: URI -> File Content
std::map<std::string, std::string> documents;

struct LspRange {
	u32 start_line;
	u32 start_char;
	u32 end_line;
	u32 end_char;
};

LspRange token_to_range(const frontend::Token &t) {
	u32 start_l = (t.location.line > 0) ? t.location.line - 1 : 0;
	u32 start_c = (t.location.column > 0) ? t.location.column - 1 : 0;

	return {start_l, start_c, start_l, start_c + static_cast<u32>(t.text.size())};
}

static json read_message() {
	std::string line;
	size_t content_length = 0;

	while (std::getline(std::cin, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		if (line.empty())
			break;

		if (line.starts_with("Content-Length: ")) {
			try {
				content_length = std::stoul(line.substr(16));
			} catch (...) {
			}
		}
	}

	if (content_length == 0)
		return nullptr;

	std::string payload(content_length, '\0');
	std::cin.read(payload.data(), (std::streamsize)content_length);

	try {
		return json::parse(payload);
	} catch (...) {
		return nullptr;
	}
}

static void send_message(const json &msg) {
	std::string body = msg.dump();
	std::cout << "Content-Length: " << body.size() << "\r\n\r\n";
	std::cout << body << std::flush;
}

static void publish_diagnostics(const std::string &uri, const std::vector<frontend::SemanticError> &errs) {
	json diagnostics = json::array();

	for (const auto &e : errs) {
		auto r = token_to_range(e.token);
		diagnostics.push_back(
			{{"range",
			  {{"start", {{"line", r.start_line}, {"character", r.start_char}}},
			   {"end", {{"line", r.end_line}, {"character", r.end_char}}}}},
			 {"severity", 1}, // 1 = Error
			 {"message", e.message}}
		);
	}

	send_message(
		{{"jsonrpc", "2.0"},
		 {"method", "textDocument/publishDiagnostics"},
		 {"params", {{"uri", uri}, {"diagnostics", diagnostics}}}}
	);
}

static void analyze_source(const std::string &uri, const std::string &text) {
	try {
		frontend::Lexer lexer(text);
		auto tokens = lexer.tokenize();
		frontend::Parser parser(tokens);
		auto unit = parser.parse();
		frontend::Semantic semantic(unit);
		auto _ = semantic.analyze();

		publish_diagnostics(uri, semantic.errors());
	} catch (...) {
	}
}

int main() {
	while (true) {
		if (std::cin.eof())
			break;

		json msg = read_message();
		if (msg.is_null())
			continue;

		try {
			if (!msg.contains("method"))
				continue;

			const std::string method = msg["method"];

			if (method == "initialize") {
				send_message(
					{{"jsonrpc", "2.0"},
					 {"id", msg["id"]},
					 {"result", {{"capabilities", {{"textDocumentSync", 1}, {"documentHighlightProvider", true}}}}}}
				);
			}

			else if (method == "shutdown") {
				send_message({{"jsonrpc", "2.0"}, {"id", msg["id"]}, {"result", nullptr}});
			} else if (method == "exit") {
				break;
			}

			else if (method == "textDocument/didOpen") {
				std::string uri = msg["params"]["textDocument"]["uri"];
				std::string text = msg["params"]["textDocument"]["text"];

				documents[uri] = text;
				analyze_source(uri, text);
			}

			else if (method == "textDocument/didChange") {
				std::string uri = msg["params"]["textDocument"]["uri"];
				std::string text = msg["params"]["contentChanges"][0]["text"];

				documents[uri] = text;
				analyze_source(uri, text);
			}

			else if (method == "textDocument/documentHighlight") {
				std::string uri = msg["params"]["textDocument"]["uri"];

				if (documents.count(uri)) {
					send_message({{"jsonrpc", "2.0"}, {"id", msg["id"]}, {"result", json::array()}});
				} else {
					send_message(
						{{"jsonrpc", "2.0"},
						 {"id", msg["id"]},
						 {"error", {{"code", -32602}, {"message", "Document not found in cache"}}}}
					);
				}
			}

			else if (msg.contains("id")) {
				send_message(
					{{"jsonrpc", "2.0"},
					 {"id", msg["id"]},
					 {"error", {{"code", -32601}, {"message", "Method not implemented"}}}}
				);
			}

		} catch (const std::exception &e) {
			std::cerr << "LSP Panic: " << e.what() << std::endl;
		}
	}
	return 0;
}
