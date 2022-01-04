/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vmoreau <vmoreau@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/12/13 19:18:35 by vmoreau           #+#    #+#             */
/*   Updated: 2022/01/04 15:27:38 by vmoreau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./Server.hpp"

std::string ipToString(unsigned int ip)
{
	unsigned char		v[4];
	std::ostringstream	os;

	for (int i = 0; i < 4; ++i)
	{
		v[i] = (ip >> (i * 8)) & 0xFF;
	}

	for (int i = 0; i < 3; ++i)
	{
		os << static_cast<int>(v[i]);
		os << '.';
	}
	os << static_cast<int>(v[3]);

	return (os.str());
}

int Server::server_is_alive = 1;

Server::Server(/* args */)
{

}

Server::~Server()
{
}

void Server::Server_closeSocket(int socket)
{
	if (socket >= 0)
		close(socket);
}

void Server::Server_closeAllSocket()
{
	for (std::map< int, sockaddr_in >::iterator it = this->_socket.begin(); it != this->_socket.end(); it++)
		this->Server_closeSocket(it->first);
	std::cout << YELLOW << "All opened Sockets have been closed.\n" << NC;
}

void Server::Server_setSocket()
{
	for (size_t i = 0; i < this->_servers.size(); i++)
	{
		int	mysock;
		int opt = 1;

		memset((char *)&mysock, 0, sizeof(mysock));
		if ((mysock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			this->Server_closeAllSocket();
			throw ServerError("Socket creation error");
		}
		if (fcntl(mysock, F_SETFL, O_NONBLOCK) < 0)
			std::cout << "Fcntl failed. errno: " << errno << std::endl;

		if (setsockopt(mysock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		{
			this->Server_closeAllSocket();
			throw ServerError("Setsockopt \"SO_REUSEADDR\" error");
		}
		if (setsockopt(mysock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
		{
			this->Server_closeAllSocket();
			throw ServerError("Setsockopt \"SO_REUSEPORT\" error");
		}

		struct sockaddr_in		myaddr;

		int addr_size = sizeof(myaddr);
		memset((char *)&myaddr, 0, addr_size);
		myaddr.sin_family = AF_INET;
		myaddr.sin_port = htons(this->_servers[i].get_port());
		myaddr.sin_addr.s_addr = inet_addr(this->_servers[i].get_host().c_str());

		std::cerr << "IP: " << ipToString(myaddr.sin_addr.s_addr) << std::endl;
		std::cerr << "PORT: " << this->_servers[i].get_port() << std::endl;

		if (bind(mysock, (struct sockaddr *)&myaddr, addr_size) < 0)
		{
			this->Server_closeAllSocket();
			throw ServerError(std::strerror(errno));
		}

		int	backlog = 1;

		if (listen(mysock, backlog) < 0)
		{
			this->Server_closeAllSocket();
			throw ServerError("Couldn't listen to socket.");
		}
		this->_socket.insert(std::make_pair(mysock, myaddr));
	}
}

void Server::Server_setFd()
{
	FD_ZERO(&this->_all_sock);
	for (std::map< int, sockaddr_in >::iterator it = this->_socket.begin(); it != this->_socket.end(); it++)
		FD_SET(it->first, &this->_all_sock);
	this->_nfds = this->_socket.rbegin()->first;
}


void Server::Server_init(confpars *html, std::vector< serv_block > serv)
{
	this->_html = html;
	this->_servers = serv;
}

void Server::Server_select()
{
	this->_readfds = this->_all_sock;
	this->_writefds = this->_all_sock;
	int ret_select = select(this->_nfds + 1, &this->_readfds, &this->_writefds, NULL, NULL);

	if (ret_select < 0 && (errno != EINTR))
		throw ServerError(std::strerror(errno));
	this->_rdy_fd = ret_select;
}

void Server::Server_loopServ()
{
	for (std::map< int, sockaddr_in >::iterator it = this->_socket.begin(); it != this->_socket.end(); it++)
	{
		if (FD_ISSET(it->first, &this->_readfds))
		{
			std::cout << PURPLE << "PASS  " << it->first << '\n' << NC;
			this->_rdy_fd--;
			int	clientSocket = accept(it->first, NULL, NULL);

			if (clientSocket < 0 && errno != EAGAIN)
			{
				std::cerr << RED << "Server error: " << NC << "Couldn't accept connection.\n";
				std::cerr << std::strerror(errno);
				exit(EXIT_FAILURE);
			}
			else if (clientSocket > 0)
			{
				std::cout << YELLOW << "New incoming connection (fd " << clientSocket << ")" << NC << std::endl;
				FD_SET(clientSocket, &this->_all_sock);
				this->_client_fd.push_back(clientSocket);
				if (clientSocket > this->_nfds)
					this->_nfds = clientSocket;
			}
		}
	}
}

void Server::Server_loopClient()
{
	for (std::vector<int>::iterator it = this->_client_fd.begin(); it != this->_client_fd.end() && this->_rdy_fd > 0;)
	{
		if (FD_ISSET(*it, &this->_readfds))
		{
			// char buffer[30001] = {0};
			// recv(*it, buffer, 30000, MSG_DONTWAIT);
			// // std::cout << RED << buffer << NC << std::endl;
			// std::string	buf = buffer;
			// Request	firstRequest(buf);
			// buf.clear();
			// if (FD_ISSET(*it, &this->_writefds))
			// {
			// 	std::ifstream ms;
			// 	ms.close();
			// 	ms.open(firstRequest.getUrl());
			// 	if (ms.is_open() == false)
			// 	{
			// 		ms.close(); // Because we're goning to open another
			// 		ms.open(this->_servers[0].get_error_page().find("404")->second);
			// 		if (ms.is_open() == false)
			// 			std::cout << "I can't open html\n";
			// 		else
			// 			std::cout << "The html is open!\n";
			// 		ms.close();

			// 		std::string url(this->_servers[0].get_error_page().find("404")->second);
			// 		Resp2	failure("HTTP/1.1", "400", url);
			// 		send(*it, failure.respond(), failure.getResponse().size() , MSG_DONTWAIT);
			// 	}
			// 	else
			// 	{
			// 		std::cout << "FOUND\n";
			// 		Resp2	failure("HTTP/1.1", "200", firstRequest.getUrl());
			// 		send(*it, failure.respond(), failure.getResponse().size() , MSG_DONTWAIT);
			// 		ms.close();
			// 	}
			// 	// FD_CLR(*it++, &_all_sock);
			// }
		}
		it++;
	}
}

void Server::Server_launch()
{
		this->_rdy_fd = 0;
		Server_setSocket();
		Server_setFd();

		while (Server::server_is_alive)
		{
			this->Server_select();

			this->Server_loopServ();
			this->Server_loopClient();
		}
}
