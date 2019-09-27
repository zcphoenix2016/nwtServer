#pragma once

#include <string>

using std::string;

class User {
public:
    User(unsigned int account, string nickname, string password, string email, string mobile)
        : m_account(account), m_nickname(nickname), m_password(password), m_email(email), m_mobile(mobile)
    {
    }

public:
    unsigned int m_account;
    string m_nickname;
    string m_password;
    string m_email;
    string m_mobile;
};