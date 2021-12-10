/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serv_block.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vmoreau <vmoreau@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/12/08 14:44:06 by vmoreau           #+#    #+#             */
/*   Updated: 2021/12/10 16:20:00 by vmoreau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "serv_block.hpp"

	// for (size_t i = 0; i < tmp.size(); i++)
	// 	std::cout << BLUE << tmp[i] << NC << '\n';

	// for (size_t i = 0; i < this->_loc_block.size(); i++)
	// 	for (size_t j = 0; j < this->_loc_block[i].size(); j++)
	// 		std::cout << YELLOW << "	" << this->_loc_block[i][j] << NC << '\n';


serv_block::serv_block() : _path(CONF_DEFAULT_PATH), _port(-1), _client_max_body_size(BODY_DEFAULT_SIZE)
{
	// std::cout << "Serv_Block construct\n";
}

serv_block::~serv_block()
{
}

// ********************* PARSING ********************* //

void serv_block::pars_serv(std::vector<std::string> block, std::string path)
{
	this->_block = block;
	this->_path = path;
	std::vector<std::string> tmp(this->_block);
	size_t nb_loc = 0;

	for (size_t i = 0; i < this->_block.size(); i++)
		if (this->_block[i].compare("location") == 0)
			nb_loc++;

	if (nb_loc == 0)
		throw NoLocationFound(this->_path);
	tmp = this->separate_loc_block(tmp, nb_loc);

	size_t i = 2;
	while (i < tmp.size() - 1)
	{
		std::string key(tmp[i].begin(), tmp[i].begin() + tmp[i].find_first_of(' '));
		std::string value(tmp[i].begin() + key.size(), tmp[i].end());

		if (key.compare("listen") == 0)
			this->set_port_host(value);
		else if (key.compare("server_name") == 0)
			this->set_server_name(value);
		else if (key.compare("client_max_body_size") == 0)
			this->set_client_max_body_size(value);
		i++;
	}

	for (size_t i = 0; i < nb_loc; i++)
	{
		loc_block tmp;

		tmp.pars_loc(this->_loc_block[i], this->_path);
		this->_location.push_back(tmp);
		std::cout << '\n';
	}

	// // DEBUG
	// std::cout << "Port: " << this->_port << "\n";
	// std::cout << "Host: " << this->_host << "\n";
	// std::cout << "Server name: " << this->_server_name << "\n";
	// std::cout << "Client Max Body Size: " << this->_client_max_body_size << "\n";
}

// ********************* SETTER ********************** //

void serv_block::set_port_host(std::string value)
{
	std::string host(value.begin()+ value.find_first_not_of(' '), value.begin() + value.find_first_of(':'));
	std::string port(value.begin() + value.find_first_of(':') + 1, value.end());

	for (size_t i = 0; i < port.size(); i++)
		if (!isdigit(port[i]))
			throw ConfFile(" Port " + port + " is not digit");

	for (size_t i = 0; i < host.size(); i++)
		if (!isdigit(host[i]))
			if (host[i] != '.')
				throw ConfFile(" Host " + host + " is not digit");
	check_host(host);
	this->_port = atoi(port.c_str());
	this->_host = host;
}

void serv_block::set_server_name(std::string value)
{
	std::string s_name(value.begin() + value.find_first_not_of(' '), value.end());

	this->_server_name = s_name;
}
void serv_block::set_client_max_body_size(std::string value)
{
	std::string c_m_b_s(value.begin() + value.find_first_not_of(' '), value.end());

	for (size_t i = 0; i < c_m_b_s.size(); i++)
		if (!isdigit(c_m_b_s[i]))
			throw ConfFile(" Client Max Body Size " + c_m_b_s + " is not digit");

	this->_client_max_body_size = atoi(c_m_b_s.c_str());
}

// ************** INTERNE FUNCTION PARS ************** //

std::vector<std::string> serv_block::save_loc_block(std::vector<std::string> tmp, size_t *pos)
{
	size_t i = *pos;
	std::vector<std::string> ret;
	int bracket = 0;

	ret.push_back(tmp[i++]);
	if (tmp[i].compare("{") != 0)
		throw ConfFile(this->_path + " Location block \"{\" Missing\n");
	bracket++;
	ret.push_back(tmp[i++]);
	while (i < tmp.size() && bracket != 0)
	{
		if (tmp[i].compare("location") == 0)
			throw ConfFile(this->_path + " Location block \"}\" Missing\n");
		if (tmp[i].compare("{") == 0)
			bracket++;
		if (tmp[i].compare("}") == 0)
			bracket--;
		ret.push_back(tmp[i]);
		i++;
	}
	if (i != tmp.size() && (tmp[i].compare("location") != 0 && tmp[i].compare("}") != 0))
		throw ConfFile(this->_path + " Location block \"{\" Missing\n");
	*pos = i;
	return (ret);
}

std::vector<std::string> serv_block::separate_loc_block(std::vector<std::string> tmp, size_t nb_loc)
{
	std::vector<std::string>ret;

	size_t i = 0;

	while (nb_loc > 0)
	{
		while (i < tmp.size() && tmp[i].compare("location"))
		{
			ret.push_back(tmp[i]);
			i++;
		}
		this->_loc_block.push_back(this->save_loc_block(tmp , &i));
		nb_loc--;
	}
	if (i == tmp.size() || (tmp[i].compare("}") != 0))
		throw ConfFile(this->_path + " Server or last location block \"}\" Missing\n");
	ret.push_back(tmp[i++]);
	if (i != tmp.size())
		throw ConfFile(this->_path + " Location block \"{\" Missing\n");

	return (ret);
}

void serv_block::check_host(std::string value)
{
	std::vector<std::string> host;

	int last_dot_pos = -1;

	for (size_t i = 0; i < value.size(); i++)
	{
		if (value[i] == '.')
		{
			std::string tmp(value.begin() + last_dot_pos + 1, value.begin() + i);
			host.push_back(tmp);
			last_dot_pos = i;
		}
		if (i + 1 == value.size())
		{
			std::string tmp(value.begin() + last_dot_pos + 1, value.begin() + i + 1);
			host.push_back(tmp);
		}
	}

	for (size_t i = 0; i < host.size(); i++)
	{
		int val = atoi(host[i].c_str());
		if (i == 0)
		{
			if (val != 127)
				throw ConfFile(" Host first byte must be 127");
		}
		else
		{
			if (val < 0 || val > 255)
				throw ConfFile(" Host must be between 127.0.0.1 && 127.255.255.255");
		}
	}
}
