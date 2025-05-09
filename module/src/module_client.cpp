
#include "module_includes.h"

#include <regex>


namespace lua_module
{

bool fromS7Address(std::string adrStr, S7Address& adrInfo)
{
    std::locale loc;
    for (std::string::size_type i=0;i<adrStr.length();++i)
        adrStr[i] = std::toupper(adrStr[i],loc);

    // remove all spaces from address string
    adrStr.erase(remove_if(adrStr.begin(), adrStr.end(), isspace), adrStr.end());

    // check for pointer (only BYTE-Pointers for now...)
    bool isPointer = false;
    if (adrStr.rfind("P#",0) == 0)
    {
        int pointerDataLen = 0;

        auto idx = adrStr.find("BYTE",0);
        if (idx != std::string::npos)
        {
            isPointer = true;

            std::string len = adrStr.substr(idx+4);
            pointerDataLen = std::stoi(len);

            // remove BYTExxxx
            adrStr = adrStr.substr(0, idx);
        }

        // remove 'P#' and let address be resolved as usual
        adrStr = adrStr.substr(2);

        adrInfo.isPointer = true;
        adrInfo.amount = pointerDataLen;
        adrInfo.wordLen = S7WLByte;
    }

    // check for single bits (EAM/IQF x.y)
    {
        std::regex re("^([EIAQMF])(\\d+)\\.(\\d+)$", std::regex_constants::ECMAScript);
        std::smatch match;
        if (std::regex_search(adrStr, match, re) && match.size() >= 4)
        {
            std::string ar = match[1]; // => capture 0 is the whole string!
            std::string by = match[2];
            std::string bi = match[3];

            if (ar == "M" || ar == "F")
                adrInfo.area = S7AreaMK;
            else if (ar == "E" || ar == "I")
                adrInfo.area = S7AreaPE;
            else if (ar == "A" || ar == "Q")
                adrInfo.area = S7AreaPA;

            adrInfo.db      = 0;
            adrInfo.start   = (std::stoi(by) * 8) + std::stoi(bi); // => start is the bit-Index here

            if (!isPointer)
            {
                adrInfo.wordLen = S7WLBit;
                adrInfo.amount  = 1;
            }

            return true;
        }
    }

    // check for byte/word/dword (EAM/IQF B/W/D x)
    {
        std::regex re("^([EIAQMF])([BWD])(\\d+)$", std::regex_constants::ECMAScript);
        std::smatch match;
        if (std::regex_search(adrStr, match, re) && match.size() >= 4)
        {
            std::string ar = match[1]; // => capture 0 is the whole string!
            std::string wi = match[2];
            std::string by = match[3];

            if (ar == "M" || ar == "F")
                adrInfo.area = S7AreaMK;
            else if (ar == "E" || ar == "I")
                adrInfo.area = S7AreaPE;
            else if (ar == "A" || ar == "Q")
                adrInfo.area = S7AreaPA;

            adrInfo.db      = 0;
            adrInfo.start   = std::stoi(by);

            if (!isPointer)
            {
                adrInfo.amount  = 1;

                if (wi == "B")
                    adrInfo.wordLen = S7WLByte;
                else if (wi == "W")
                    adrInfo.wordLen = S7WLWord;
                else if (wi == "D")
                    adrInfo.wordLen = S7WLDWord;
            }


            return true;
        }
    }

    // check for single bits in data block (DBx.DBXy.z)
    {
        std::regex re("^DB(\\d+)[.]DBX(\\d+)[.](\\d+)$", std::regex_constants::ECMAScript);
        std::smatch match;
        if (std::regex_search(adrStr, match, re) && match.size() >= 4)
        {
            std::string db = match[1]; // => capture 0 is the whole string!
            std::string by = match[2];
            std::string bi = match[3];

            adrInfo.area    = S7AreaDB;
            adrInfo.db      = std::stoi(db);
            adrInfo.start   = (std::stoi(by) * 8) + std::stoi(bi); // => start is the bit-Index here

            if (!isPointer)
            {
                adrInfo.wordLen = S7WLBit;
                adrInfo.amount  = 1;
            }

            return true;
        }
    }

    // check for byte/word/dword in data block (DBx.DB B/W/D y)
    {
        std::regex re("^DB(\\d+)[.]DB([BWD])(\\d+)$", std::regex_constants::ECMAScript);
        std::smatch match;
        if (std::regex_search(adrStr, match, re) && match.size() >= 4)
        {
            std::string db = match[1]; // => capture 0 is the whole string!
            std::string wi = match[2];
            std::string by = match[3];

            adrInfo.area    = S7AreaDB;
            adrInfo.db      = std::stoi(db);
            adrInfo.start   = std::stoi(by);


            if (!isPointer)
            {
                adrInfo.amount  = 1;

                if (wi == "B")
                    adrInfo.wordLen = S7WLByte;
                else if (wi == "W")
                    adrInfo.wordLen = S7WLWord;
                else if (wi == "D")
                    adrInfo.wordLen = S7WLDWord;
            }


            return true;
        }
    }

    return false;
}

sol::variadic_results read(TS7Client& client, std::string address, sol::optional<S7FormatHint> formatHint, sol::optional<int> amount, sol::this_state L)
{
    sol::variadic_results values;

    // check and interpret given address
    S7Address adrInfo;
    bool valid = fromS7Address(address, adrInfo);
    if (!valid)
    {
        values.push_back({ L, sol::in_place, sol::lua_nil });
        values.push_back({ L, sol::in_place, "Invalid address string!" });
        return values;
    }

    if (amount)
        adrInfo.amount = amount.value();
    else
        amount = 1;

    // take default/given format hint
    S7FormatHint hint = S7FormatHint::Unsigned;
    if (formatHint)
    {
        hint = formatHint.value();
    }


    uint8_t buf[0xFFFF];
    int ret = client.ReadArea(adrInfo.area, adrInfo.db, adrInfo.start, adrInfo.amount, adrInfo.wordLen, &buf);
    if (ret != 0)
    {
        values.push_back({ L, sol::in_place, sol::lua_nil });
        values.push_back({ L, sol::in_place, CliErrorText(ret) });

        return values;
    }

    if (adrInfo.isPointer)
    {
        std::string s((const char*)buf, adrInfo.amount);
        values.push_back({ L, sol::in_place, s });
    }
    else if (adrInfo.wordLen == S7WLBit)
    {
        for (int i = 0; i < amount; i++) {
            values.push_back({ L, sol::in_place_type<bool>, buf[i] > 0 });
        };
    }
    else if (adrInfo.wordLen == S7WLByte)
    {
        if (hint == S7FormatHint::Signed) {
            for (int i = 0; i < amount; i++) {
                values.push_back({ L, sol::in_place_type<int8_t>, buf[i] });
            };
        }
        else
            for (int i = 0; i < amount; i++) {
                values.push_back({ L, sol::in_place_type<uint8_t>, buf[i] });
            };
    }
    else if (adrInfo.wordLen == S7WLWord)
    {
        if (hint == S7FormatHint::Signed)
        {
            for (int i = 0; i < amount; i++) {
                int16_t v = (*(int16_t*)&buf[i]);
                SwapEndian(v);
                values.push_back({ L, sol::in_place_type<int16_t>, v });
            };
        }
        else
        {
            for (int i = 0; i < amount; i++) {
                uint16_t v = (*(uint16_t*)&buf[i]);
                SwapEndian(v);
                values.push_back({ L, sol::in_place_type<uint16_t>, v });
            };
        }
    }
    else if (adrInfo.wordLen == S7WLDWord)
    {
        if (hint == S7FormatHint::Float)
        {
            for (int i = 0; i < amount; i++) {
                float v = (*(float*)&buf[i]);
                SwapEndian(v);
                values.push_back({ L, sol::in_place_type<float>, v });
            };
        }
        else if (hint == S7FormatHint::Signed)
        {
            for (int i = 0; i < amount; i++) {
                int32_t v = (*(int32_t*)&buf[i]);
                SwapEndian(v);
                values.push_back({ L, sol::in_place_type<int32_t>, v });
            };
        }
        else
        {
            for (int i = 0; i < amount; i++) {
                uint32_t v = (*(uint32_t*)&buf[i]);
                SwapEndian(v);
                values.push_back({ L, sol::in_place_type<uint32_t>, v });
            };
        }
    }
    else
    {
        values.push_back({ L, sol::in_place, sol::lua_nil });
        values.push_back({ L, sol::in_place, "Invalid data!" });
    }

    return values;
}

sol::variadic_results write(TS7Client& client, std::string address, sol::object value, sol::this_state L)
{
    sol::variadic_results values;

    // check and interpret given address
    S7Address adrInfo;
    bool valid = fromS7Address(address, adrInfo);
    if (!valid)
    {
        values.push_back({ L, sol::in_place, sol::lua_nil });
        values.push_back({ L, sol::in_place, "Invalid address string!" });
        return values;
    }

    uint8_t buf[0xFFFF];

    if (adrInfo.isPointer && value.is<std::string>())
    {
        std::string data = value.as<std::string>();
        int size = std::min(adrInfo.amount, static_cast<int>(data.length()));
        std::memcpy((char*)buf, data.c_str(), size);
    }
    else if (adrInfo.wordLen == S7WLBit && value.is<bool>())
    {
        buf[0] = value.as<bool>();
    }
    else if (adrInfo.wordLen == S7WLByte && value.is<int>())
    {
        int v = value.as<int>();
        if (v > 255 || v < -128)
        {
            values.push_back({ L, sol::in_place, sol::lua_nil });
            values.push_back({ L, sol::in_place, "Value out of range (0..255 / -128..+127)!" });
            return values;
        }
        buf[0] = value.as<uint8_t>();
    }
    else if (adrInfo.wordLen == S7WLWord && value.is<int>())
    {
        int vc = value.as<int>();
        if (vc > 65535 || vc < -32768)
        {
            values.push_back({ L, sol::in_place, sol::lua_nil });
            values.push_back({ L, sol::in_place, "Value out of range (0..65535 / -32768..+32767)!" });
            return values;
        }

        uint16_t v = value.as<uint16_t>();
        SwapEndian(v);
        (*(uint16_t*)&buf[0]) = v;

    }
    else if (adrInfo.wordLen == S7WLDWord && (value.is<int>() || value.is<float>()))
    {
        if (value.is<int>())
        {
            uint32_t v = value.as<uint32_t>();
            SwapEndian(v);
            (*(uint32_t*)&buf[0]) = v;
        }
        else
        {
            float v = value.as<float>();
            SwapEndian(v);
            (*(float*)&buf[0]) = v;
        }
    }
    else
    {
        values.push_back({ L, sol::in_place, sol::lua_nil });
        values.push_back({ L, sol::in_place, "Invalid data!" });
        return values;
    }


    int ret = client.WriteArea(adrInfo.area, adrInfo.db, adrInfo.start, adrInfo.amount, adrInfo.wordLen, &buf);

    values.push_back({ L, sol::in_place, ret });
    values.push_back({ L, sol::in_place, CliErrorText(ret) });

    return values;
}

void register_client(sol::table& module)
{
    module.new_usertype<TS7Client>(
        "TS7Client",            sol::constructors<TS7Client()>()

        ,"setConnectionType",   &TS7Client::SetConnectionType

        ,"connectTo",           [](TS7Client& client, const char* address, int rack, int slot)
                                {
                                    int ret = client.ConnectTo(address, rack, slot);
                                    return returnWithCliError(ret);
                                }

        ,"disconnect",          [](TS7Client& client)
                                {
                                    int ret = client.Disconnect();
                                    return returnWithCliError(ret);
                                }

        ,"isConnected",         &TS7Client::Connected
        ,"plcStatus",           &TS7Client::PlcStatus


        ,"read",                &read
        ,"write",               &write
    );
}

} // namespace lua_module
