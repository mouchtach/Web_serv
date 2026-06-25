#include "ParssingConf.hpp"
#include "Config.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
// #include <vector>

std::string intToStr(int n) {
  std::ostringstream ss;
  ss << n;
  return ss.str();
}

std::string ParssingConf::readFile(const std::string &filename) {
  std::ifstream file(filename.c_str());
  if (!file.is_open())
    error("Cannot open config file: '" + filename + "'");
  std::ostringstream ss;
  ss << file.rdbuf();
  std::string content = ss.str();
  if (content.empty())
    error("Config file is empty: '" + filename + "'");
  return content;
}

std::string ParssingConf::removeComments(const std::string &content) {
  std::string result;
  result.reserve(content.size());

  bool inComment = false;
  for (size_t i = 0; i < content.size(); ++i) {
    if (content[i] == '#')
      inComment = true;
    if (content[i] == '\n')
      inComment = false;
    if (!inComment)
      result += content[i];
  }
  return result;
}

std::vector<std::string> ParssingConf::tokenize(const std::string &content) {
  std::vector<std::string> tokens;
  std::string current;

  for (size_t i = 0; i < content.size(); ++i) {
    char c = content[i];

    if (c == '{' || c == '}') {
      if (!current.empty()) {
        tokens.push_back(current);
        current.clear();
      }
      tokens.push_back(std::string(1, c));
    } else if (isspace(c)) {
      if (!current.empty()) {
        tokens.push_back(current);
        current.clear();
      }
    } else {
      current += c;
    }
  }
  if (!current.empty())
    tokens.push_back(current);

  return tokens;
}

Config ParssingConf::parseServerBlock(const std::vector<std::string> &tokens, size_t &i) 
{

	Config cfg;

	while (i < tokens.size() && tokens[i] != "}") 
	{
		std::string directive = tokens[i++];
		if (directive == "listen") {
			if (i >= tokens.size())
				error("'listen' missing value");
			cfg.setPort(tokens[i++]);
		} else if (directive == "server_name") {
			if (i >= tokens.size())
				error("'server_name' missing value");
			cfg.setServerName(tokens[i++]);
		} else if (directive == "autoindex") {
			if (i >= tokens.size())
				error("'autoindex' missing value");
			cfg.setAutoindex(tokens[i++]);
		} else if (directive == "root") {
			if (i >= tokens.size())
				error("'root' missing value");
			cfg.setRoot(tokens[i++]);
		} else if (directive == "index") {
			if (i >= tokens.size())
				error("'index' missing value");
			cfg.setIndex(tokens[i++]);
		} else if (directive == "client_max_body_size") {
			if (i >= tokens.size())
				error("'client_max_body_size' missing value");
			cfg.setClientMaxBodySize(tokens[i++]);
		} else if (directive == "error_page") {
			if (i + 1 >= tokens.size())
				error("'error_page' requires code and path");
			std::string code = tokens[i++];
			std::string path = tokens[i++];
			cfg.addErrorPage(code, path);
		} else if (directive == "location") {

			if (i >= tokens.size())
				error("'location' missing path");
			std::string path = tokens[i++];
			if (!path.empty() && path[path.size() - 1] == ';')
				path = path.substr(0, path.size() - 1);
			if (i >= tokens.size() || tokens[i] != "{")
				error("Expected '{' after location path '" + path + "'");
			++i;
			LocationConfig loc(cfg);
			loc.override(tokens, i, path);
			// loc = parseLocationBlock(tokens, i, path);

			if (i >= tokens.size() || tokens[i] != "}")
				error("Expected '}' to close location block '" + path + "'");
			++i;
			cfg.addLocation(loc);
		} else if (directive == "methods") {
			if (i >= tokens.size())
				error("'methods' missing values");
			std::vector<std::string> methods;
			while (i < tokens.size() && tokens[i] != "}") {
				std::string m = tokens[i++];
				methods.push_back(m);
				if (!m.empty() && m[m.size() - 1] == ';')
					break;
			}
			cfg.setMethods(methods);
		} else {
			error("Unknown directive in server block: '" + directive + "'");
		}
	}
	if (cfg.getPort() == 0)
		error("Server block is missing 'listen' directive");
	if (cfg.getRoot().empty())
		error("Server block (port " + intToStr(cfg.getPort()) + ") is missing 'root'");

	return cfg;
}


void ParssingConf::validate() {
  for (size_t i = 0; i < _configs.size(); ++i) {
    for (size_t j = i + 1; j < _configs.size(); ++j) {
      // Two server blocks with same port AND same server_name → ambiguous
      if (_configs[i].getPort() == _configs[j].getPort() &&
          _configs[i].getServerName() == _configs[j].getServerName()) {
        error(
            "Duplicate server block: port=" + intToStr(_configs[i].getPort()) +
            " server_name=" + _configs[i].getServerName());
      }
    }
  }
}

void ParssingConf::error(const std::string &msg) {
  throw std::runtime_error("[Config Error] " + msg);
}

const std::vector<Config> &ParssingConf::getConfigs() const { return _configs; }

void ParssingConf::parseConfig(const std::string &filename) {
  // read file, remove comments, tokenize, parse into Config objects
  std::string raw = readFile(filename);
  std::string cleaned = removeComments(raw);
  std::vector<std::string> tokens = tokenize(cleaned);

  if (tokens.empty())
    error("Config file is empty: " + filename);

  size_t i = 0;
  while (i < tokens.size()) {
    if (tokens[i] == "server") {
      ++i;
      if (i >= tokens.size() || tokens[i] != "{")
        error("Expected '{' after 'server', got '" +
              (i < tokens.size() ? tokens[i] : "EOF") + "'");
      ++i;
      Config cfg = parseServerBlock(tokens, i);
      if (i >= tokens.size() || tokens[i] != "}")
        error("Expected '}' to close server block");
      ++i;
      _configs.push_back(cfg);
    } else
      error("Unexpected token outside server block: '" + tokens[i] + "'");
  }

  if (_configs.empty())
    error("No server blocks found in config file");
  validate();
}


void LocationConfig::override(const std::vector<std::string> &tokens, size_t &i, const std::string path) {

  setPath(path);

  while (i < tokens.size() && tokens[i] != "}") {
    std::string directive = tokens[i++];
    if (directive == "root") {
      if (i >= tokens.size())
        error("location '" + path + "': 'root' missing value");
      setRoot(tokens[i++]);
      rootOverridden = true;
    } else if (directive == "index") {
      if (i >= tokens.size())
        error("location '" + path + "': 'index' missing value");
      setIndex(tokens[i++]);
    } else if (directive == "autoindex") {
      if (i >= tokens.size())
        error("location '" + path + "': 'autoindex' missing value");
     setAutoindex(tokens[i++]);
    } else if (directive == "methods") {
      if (i >= tokens.size())
        error("location '" + path + "': 'methods' missing values");
      std::vector<std::string> listmethods;
      while (i < tokens.size() && tokens[i] != "}") 
	  {
		std::string m = tokens[i++];
		listmethods.push_back(m);
		if (!m.empty() && m[m.size() - 1] == ';')
		  break;
      }
      setMethods(listmethods);

    } else if (directive == "cgi_extension") {
      if (i >= tokens.size())
        error("location '" + path + "': 'cgi_extension' missing value");
      setCgiExtension(tokens[i++]);
    } else if (directive == "cgi_path") {
      if (i >= tokens.size())
        error("location '" + path + "': 'cgi_path' missing value");
      setCgiPath(tokens[i++]);
    } else if (directive == "return") {
      if (i >= tokens.size())
        error("location '" + path + "': 'return' missing value");
      setReturn(tokens, &i);
      // i++;
    } else {
      error("Unknown directive in location '" + path + "': '" + directive + "'");
    }
  }
//   return loc;
}