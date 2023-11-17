#ifndef ESL_COM_HTTP_SERVER_MHDSOCKET_H_
#define ESL_COM_HTTP_SERVER_MHDSOCKET_H_

#include <esl/com/http/server/RequestHandler.h>
#include <esl/com/http/server/Socket.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace esl {
inline namespace v1_6 {
namespace com {
namespace http {
namespace server {

class MHDSocket : public Socket {
public:
	struct Settings {
		Settings() = default;
		Settings(const std::vector<std::pair<std::string, std::string>>& settings);

		uint16_t port = 0;
		uint16_t numThreads = 4;
		unsigned int connectionTimeout = 120;
		unsigned int connectionLimit = 1000;
		unsigned int perIpConnectionLimit = 0;
	};

	MHDSocket(const Settings& settings);

	static std::unique_ptr<Socket> create(const std::vector<std::pair<std::string, std::string>>& settings);

	void addTLSHost(const std::string& hostname, std::vector<unsigned char> certificate, std::vector<unsigned char> key) override;

	/* this method is blocking. */
	void listen(const RequestHandler& requestHandler) override;

	/* this method is non-blocking. A separate thread will be opened to listen */
	void listen(const RequestHandler& requestHandler, std::function<void()> onReleasedHandler) override;

	void release() override;

private:
	std::unique_ptr<Socket> socket;
};

} /* namespace server */
} /* namespace http */
} /* namespace com */
} /* inline namespace v1_6 */
} /* namespace esl */

#endif /* ESL_COM_HTTP_SERVER_MHDSOCKET_H_ */
