#ifndef EVT_ACCOUNT_H
#define EVT_ACCOUNT_H

#include <string>

namespace evt
{

int checkAccountByUserName(const std::string& userName, const std::string& pwd, uint32_t* userId, uint8_t* type);

int checkAccountById(uint32_t id, const std::string& pwd, std::string* userName);

} // namespace evt

#endif // EVT_ACCOUNT_H
