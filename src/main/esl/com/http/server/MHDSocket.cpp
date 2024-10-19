#include <esl/com/http/server/MHDSocket.h>
#include <esl/system/Stacktrace.h>
#include <esl/utility/String.h>

#include <mhd4esl/com/http/server/Socket.h>

#include <stdexcept>

namespace esl {
inline namespace v1_6 {
namespace com {
namespace http {
namespace server {

MHDSocket::Settings::Settings(const std::vector<std::pair<std::string, std::string>>& settings) {
	bool hasHttps = false;
	bool hasThreads = false;
	bool hasConnectionTimeout = false;
	bool hasConnectionLimit = false;
	bool hasPerIpConnectionLimit = false;

	for(const auto& setting : settings) {
		if(setting.first == "https") {
			if(hasHttps) {
	            throw system::Stacktrace::add(std::runtime_error("multiple definition of attribute 'https'."));
			}
			hasHttps = true;
			https = esl::utility::String::toBool(setting.second);
		}
		else if(setting.first == "threads") {
			if(hasThreads) {
	            throw system::Stacktrace::add(std::runtime_error("multiple definition of attribute 'threads'."));
			}
			hasThreads = true;

#if 0
			int i = esl::utility::String::toNumber<int>(setting.second);
		    if(i < 0) {
		    	throw system::Stacktrace::add(std::runtime_error("Invalid negative value for \"" + setting.first + "\"=\"" + setting.second + "\""));
		    }

			numThreads = static_cast<uint16_t>(i);
#else
			numThreads = esl::utility::String::toNumber<uint16_t>(setting.second);
#endif
		    if(numThreads <= 0) {
		    	throw system::Stacktrace::add(std::runtime_error("Invalid value for \"" + setting.first + "\"=\"" + setting.second + "\""));
		    }
		}
		else if(setting.first == "port") {
			if(port != 0) {
	            throw system::Stacktrace::add(std::runtime_error("multiple definition of attribute 'port'."));
			}

			int i = utility::String::toNumber<int>(setting.second);
		    if(i < 0) {
		    	throw system::Stacktrace::add(std::runtime_error("Invalid negative value for \"" + setting.first + "\"=\"" + setting.second + "\""));
		    }

			port = static_cast<uint16_t>(i);
		    if(port == 0) {
		    	throw system::Stacktrace::add(std::runtime_error("Invalid value for \"" + setting.first + "\"=\"" + setting.second + "\""));
		    }
		}
		else if(setting.first == "connection-timeout") {
			if(hasConnectionTimeout) {
	            throw system::Stacktrace::add(std::runtime_error("multiple definition of attribute 'connection-timeout'."));
			}
			hasConnectionTimeout = true;

			int i = utility::String::toNumber<int>(setting.second);
		    if(i < 0) {
		    	throw system::Stacktrace::add(std::runtime_error("Invalid negative value for \"" + setting.first + "\"=\"" + setting.second + "\""));
		    }

			connectionTimeout = static_cast<unsigned int>(i);
		    if(connectionTimeout <= 0) {
		    	throw system::Stacktrace::add(std::runtime_error("Invalid value for \"" + setting.first + "\"=\"" + setting.second + "\""));
		    }
		}
		else if(setting.first == "connection-limit") {
			if(hasConnectionLimit) {
	            throw system::Stacktrace::add(std::runtime_error("multiple definition of attribute 'connection-limit'."));
			}
			hasConnectionLimit = true;

			int i = utility::String::toNumber<int>(setting.second);
		    if(i < 0) {
		    	throw system::Stacktrace::add(std::runtime_error("Invalid negative value for \"" + setting.first + "\"=\"" + setting.second + "\""));
		    }

			connectionLimit = static_cast<unsigned int>(i);
		    if(connectionLimit <= 0) {
		    	throw system::Stacktrace::add(std::runtime_error("Invalid value for \"" + setting.first + "\"=\"" + setting.second + "\""));
		    }
		}
		else if(setting.first == "per-ip-connection-limit") {
			if(hasPerIpConnectionLimit) {
	            throw system::Stacktrace::add(std::runtime_error("multiple definition of attribute 'per-ip-connection-limit'."));
			}
			hasPerIpConnectionLimit = true;

			int i = utility::String::toNumber<int>(setting.second);
		    if(i < 0) {
		    	throw system::Stacktrace::add(std::runtime_error("Invalid negative value for \"" + setting.first + "\"=\"" + setting.second + "\""));
		    }

			perIpConnectionLimit = static_cast<unsigned int>(i);
		    if(perIpConnectionLimit <= 0) {
		    	throw system::Stacktrace::add(std::runtime_error("Invalid value for \"" + setting.first + "\"=\"" + setting.second + "\""));
		    }
		}
		else {
			throw system::Stacktrace::add(std::runtime_error("Key \"" + setting.first + "\" is unknown"));
		}
	}

	if(port == 0) {
    	throw system::Stacktrace::add(std::runtime_error("Parameter \"port\" is missing"));
	}
}

MHDSocket::MHDSocket(const Settings& settings)
: socket(createNative(settings))
{ }

std::unique_ptr<Socket> MHDSocket::create(const std::vector<std::pair<std::string, std::string>>& settings) {
	return std::unique_ptr<Socket>(new MHDSocket(Settings(settings)));
}

std::unique_ptr<Socket> MHDSocket::createNative(const Settings& settings) {
	return std::unique_ptr<Socket>(new mhd4esl::com::http::server::Socket(settings));
}

void MHDSocket::listen(const RequestHandler& requestHandler) {
	socket->listen(requestHandler);
}

void MHDSocket::listen(const RequestHandler& requestHandler, std::function<void()> onReleasedHandler) {
	socket->listen(requestHandler, onReleasedHandler);
}

void MHDSocket::release() {
	socket->release();
}

} /* namespace server */
} /* namespace http */
} /* namespace com */
} /* inline namespace v1_6 */
} /* namespace esl */
