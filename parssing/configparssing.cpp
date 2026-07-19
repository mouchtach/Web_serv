#include "configparssing.hpp"
#include "utils.hpp"

ConfigParssing::ConfigParssing(const std::string &filename) : _filename(filename) {
}

ConfigParssing::~ConfigParssing() {
}

void ConfigParssing::ReadConfig() {
    std::ifstream file(_filename.c_str());
    if (!file.is_open())
        throw std::runtime_error("Failed to open config file: '" + _filename + "'");
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    if (content.empty())
        throw std::runtime_error("Config file is empty after removing comments: '" + _filename + "'");
    _content = content;
}

void ConfigParssing::removeComments() {
    std::string result;
    result.reserve(_content.size());
    bool inComment = false;
    for (size_t i = 0; i < _content.size(); ++i) {
        if (_content[i] == '#')
            inComment = true;
        if (_content[i] == '\n')
            inComment = false;
        if (!inComment)
            result += _content[i];
    }
    _content = result;
}

void ConfigParssing::tokenize() {
    std::string current;
    if (_content.empty())
        throw std::runtime_error("Config file is empty after removing comments: '" + _filename + "'");
    for (size_t i = 0; i < _content.size(); ++i) 
    {
        char c = _content[i];
        if (c == '{' || c == '}')
        {
            if (!current.empty()) 
            {
                _tokens.push_back(current);
                current.clear();
            }
            _tokens.push_back(std::string(1, c));
        } 
        else if (isspace(c)) 
        {
            if (!current.empty()) {
                _tokens.push_back(current);
                current.clear();
            }
        } else
            current += c;
    }
    if (!current.empty())
        _tokens.push_back(current);
}

void ConfigParssing::parseConfig() {
    size_t i = 0;
    while (i < _tokens.size()) {
        if (_tokens[i] == "server") {
            ++i;
        if (i >= _tokens.size() || _tokens[i] != "{")
            throw std::runtime_error("Expected '{' after 'server', got '" + (i < _tokens.size() ? _tokens[i] : "EOF") + "'");
        ++i;
        parseServer(i);
        if (i >= _tokens.size() || _tokens[i] != "}")
            throw std::runtime_error("Expected '}' to close server block");
        ++i;
        } else
            throw std::runtime_error("Unexpected token outside server block: '" + _tokens[i] + "'");
    }
    if (_configs.empty())
        throw std::runtime_error("No server blocks found in config file");
}

void ConfigParssing::parseServer(size_t &i) 
{
    Config cfg;
    while (i < _tokens.size() && _tokens[i] != "}") 
    {
		std::string directive = _tokens[i++];
		if (directive == "listen") {
			if (i >= _tokens.size())
				throw std::runtime_error("'listen' missing value");
			cfg.setPort(_tokens[i++]);
		} else if (directive == "server_name") {
			if (i >= _tokens.size())
				throw std::runtime_error("'server_name' missing value");
			cfg.setServerName(_tokens[i++]);
		} else if (directive == "autoindex") {
			if (i >= _tokens.size())
				throw std::runtime_error("'autoindex' missing value");
			cfg.setAutoindex(_tokens[i++]);
		} else if (directive == "root") {
			if (i >= _tokens.size())
				throw std::runtime_error("'root' missing value");
			cfg.setRoot(_tokens[i++]);
		} else if (directive == "index") {
			if (i >= _tokens.size())
				throw std::runtime_error("'index' missing value");
			cfg.setIndex(_tokens[i++]);
		} else if (directive == "client_max_body_size") {
			if (i >= _tokens.size())
				throw std::runtime_error("'client_max_body_size' missing value");
			cfg.setClientMaxBodySize(_tokens[i++]);
		} else if (directive == "error_page") {
			if (i + 1 >= _tokens.size())
				throw std::runtime_error("'error_page' requires code and path");
			std::string code = _tokens[i++];
			std::string path = _tokens[i++];
			cfg.addErrorPage(code, path);
		} else if (directive == "location") {
			if (i >= _tokens.size())
				throw std::runtime_error("'location' missing path");
			std::string path = _tokens[i++];
			if (!path.empty() && path[path.size() - 1] == ';')
				path = path.substr(0, path.size() - 1);
			if (i >= _tokens.size() || _tokens[i] != "{")
				throw std::runtime_error("Expected '{' after 'location " + path + "'");
			++i;
			Location loc(cfg);
            loc.setPath(path);
			loc.overrideLocation(_tokens, i);
			if (i >= _tokens.size() || _tokens[i] != "}")
				throw std::runtime_error("Expected '}' to close location block '" + path + "'");
			++i;
			cfg.addLocation(loc);
		} else if (directive == "methods") {
			if (i >= _tokens.size())
				throw std::runtime_error("'methods' missing values");
			std::vector<std::string> methods;
			while (i < _tokens.size() && _tokens[i] != "}") {
				std::string m = _tokens[i++];
				methods.push_back(m);
				if (!m.empty() && m[m.size() - 1] == ';')
					break;
			}
			cfg.setMethods(methods);
		} else {
			throw std::runtime_error("Unknown directive in server block: '" + directive + "'");
		}
	}
	if (cfg.getPort() == 0)
		throw std::runtime_error("Server block is missing 'listen' directive");
	if (cfg.getRoot().empty())
		throw std::runtime_error("Server block (port " + intToStr(cfg.getPort()) + ") is missing 'root'");
    _configs.push_back(cfg);
}


void ConfigParssing::validate() {
    for (size_t i = 0; i < _configs.size(); ++i) {
        for (size_t j = i + 1; j < _configs.size(); ++j) {
            if (_configs[i].getPort() == _configs[j].getPort() && _configs[i].getServerName() == _configs[j].getServerName()) {
                throw std::runtime_error( "Duplicate server block: port=" + intToStr(_configs[i].getPort()) + " server_name=" + _configs[i].getServerName());
            }
        }
    }
}

std::vector<Config> ConfigParssing::getConfigs() const {
    return _configs;
}