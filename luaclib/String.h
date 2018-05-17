/*
\brief 文件描述需要修改
\file String.h

Date:2017年5月6日
Author:揭育龙

Copyright (c) 2017 创娱游戏. All rights reserved.
*/

#ifndef _STRING_H_734083D4_72FA_43E9_A697_DEEA47F4CA97
#define _STRING_H_734083D4_72FA_43E9_A697_DEEA47F4CA97

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdexcept>

class Exception
{
public:
	enum 
	{
		ERR_INTERNAL_ERROR
	};

	Exception(int code, const std::string& msg):
		mMsg(msg) {};
	int code() const { return mCode; }
	const std::string& msg() const { return mMsg; }

private:
	int mCode;
	std::string mMsg;
};

#define STRING_THROW_EXCEPT(code, msg) throw Exception(code, msg)

#define JENGINE_API
#define STR_QUICKBUFF_SIZE	32
//类型定义
typedef unsigned int uint;
typedef		uint8_t	utf8;
typedef		uint32_t	utf32;
//字符串类
class JENGINE_API String
{
public:
	//整型类型
	typedef		utf32			value_type;
	typedef		uint32_t			size_type;
	typedef		std::ptrdiff_t	difference_type;
	typedef		utf32&			reference;
	typedef		const utf32&	const_reference;
	typedef		utf32*			pointer;
	typedef		const utf32*	const_pointer;
	static const size_type		npos;

private:
	size_type	mCplength;
	size_type	mReserve;

	mutable utf8*		mEncodedbuff;
	mutable size_type	mEncodeddatlen;
	mutable size_type	mEncodedbufflen;

	utf32		mQuickbuff[STR_QUICKBUFF_SIZE];
	utf32*		mBuffer;

public:
	//迭代器
    class iterator : public std::iterator<std::random_access_iterator_tag, utf32>
    {
    public:
        iterator() : mPtr(0) {}
        explicit iterator(utf32* const ptr) : mPtr(ptr) {}

        utf32& operator*() const
        {
            return *mPtr;
        }

        utf32* operator->() const
        {
            return &**this;
        }

        String::iterator& operator++()
        {
            ++mPtr;
            return *this;
        }

        String::iterator operator++(int)
        {
            String::iterator temp = *this;
            ++*this;
            return temp;
        }

        String::iterator& operator--()
        {
            --mPtr;
            return *this;
        }

        String::iterator operator--(int)
        {
            String::iterator temp = *this;
            --*this;
            return temp;
        }

        String::iterator& operator+=(difference_type offset)
        {
            mPtr += offset;
            return *this;
        }

        String::iterator operator+(difference_type offset) const
        {
            String::iterator temp = *this;
            return temp += offset;
        }

        String::iterator& operator-=(difference_type offset)
        {
            return *this += -offset;
        }

        String::iterator operator-(difference_type offset) const
        {
            String::iterator temp = *this;
            return temp -= offset;
        }

        utf32& operator[](difference_type offset) const
        {
            return *(*this + offset);
        }

        friend difference_type operator-(const String::iterator& lhs,
                                         const String::iterator& rhs)
            { return lhs.mPtr - rhs.mPtr; }

        friend String::iterator operator+(difference_type offset, const String::iterator& iter)
            { return iter + offset; }

        friend bool operator==(const String::iterator& lhs,
                               const String::iterator& rhs)
            { return lhs.mPtr == rhs.mPtr; }

        friend bool operator!=(const String::iterator& lhs,
                               const String::iterator& rhs)
            { return lhs.mPtr != rhs.mPtr; }

        friend bool operator<(const String::iterator& lhs,
                              const String::iterator& rhs)
            { return lhs.mPtr < rhs.mPtr; }

        friend bool operator>(const String::iterator& lhs,
                              const String::iterator& rhs)
            { return lhs.mPtr > rhs.mPtr; }

        friend bool operator<=(const String::iterator& lhs,
                               const String::iterator& rhs)
            { return lhs.mPtr <= rhs.mPtr; }

        friend bool operator>=(const String::iterator& lhs,
                               const String::iterator& rhs)
            { return lhs.mPtr >= rhs.mPtr; }

        utf32* mPtr;
    };

    //常量迭代器
    class const_iterator : public std::iterator<std::random_access_iterator_tag, const utf32>
    {
    public:
        const_iterator() : mPtr(0) {}
        explicit const_iterator(const utf32* const ptr) : mPtr(ptr) {}
        const_iterator(const String::iterator& iter) : mPtr(iter.mPtr) {}

        const utf32& operator*() const
        {
            return *mPtr;
        }

        const utf32* operator->() const
        {
            return &**this;
        }

        String::const_iterator& operator++()
        {
            ++mPtr;
            return *this;
        }

        String::const_iterator operator++(int)
        {
            String::const_iterator temp = *this;
            ++*this;
            return temp;
        }

        String::const_iterator& operator--()
        {
            --mPtr;
            return *this;
        }

        String::const_iterator operator--(int)
        {
            String::const_iterator temp = *this;
            --*this;
            return temp;
        }

        String::const_iterator& operator+=(difference_type offset)
        {
            mPtr += offset;
            return *this;
        }

        String::const_iterator operator+(difference_type offset) const
        {
            String::const_iterator temp = *this;
            return temp += offset;
        }

        String::const_iterator& operator-=(difference_type offset)
        {
            return *this += -offset;
        }

        String::const_iterator operator-(difference_type offset) const
        {
            String::const_iterator temp = *this;
            return temp -= offset;
        }

        const utf32& operator[](difference_type offset) const
        {
            return *(*this + offset);
        }

        String::const_iterator& operator=(const String::iterator& iter)
        {
            mPtr = iter.mPtr;
            return *this;
        }

        friend String::const_iterator operator+(difference_type offset, const String::const_iterator& iter)
            { return iter + offset; }

        friend difference_type operator-(const String::const_iterator& lhs,
                                         const String::const_iterator& rhs)
            { return lhs.mPtr - rhs.mPtr; }

        friend bool operator==(const String::const_iterator& lhs,
                               const String::const_iterator& rhs)
            { return lhs.mPtr == rhs.mPtr; }

        friend bool operator!=(const String::const_iterator& lhs,
                               const String::const_iterator& rhs)
            { return lhs.mPtr != rhs.mPtr; }

        friend bool operator<(const String::const_iterator& lhs,
                              const String::const_iterator& rhs)
            { return lhs.mPtr < rhs.mPtr; }

        friend bool operator>(const String::const_iterator& lhs,
                              const String::const_iterator& rhs)
            { return lhs.mPtr > rhs.mPtr; }

        friend bool operator<=(const String::const_iterator& lhs,
                               const String::const_iterator& rhs)
            { return lhs.mPtr <= rhs.mPtr; }

        friend bool operator>=(const String::const_iterator& lhs,
                               const String::const_iterator& rhs)
            { return lhs.mPtr >= rhs.mPtr; }

        const utf32* mPtr;
    };

	//常量反向迭代器定义
#if defined(_MSC_VER) && ((_MSC_VER <= 1200) || ((_MSC_VER <= 1300) && defined(_STLPORT_VERSION)))
	typedef	std::reverse_iterator<const_iterator, const_pointer, const_reference, difference_type>	const_reverse_iterator;
#else
	typedef	std::reverse_iterator<const_iterator>	const_reverse_iterator;
#endif

	//反向迭代器定义
#if defined(_MSC_VER) && ((_MSC_VER <= 1200) || ((_MSC_VER <= 1300) && defined(_STLPORT_VERSION)))
	typedef std::reverse_iterator<iterator, pointer, reference, difference_type>			reverse_iterator;
#else
	typedef std::reverse_iterator<iterator>			reverse_iterator;
#endif

public:
    //快速比较
    struct FastLessCompare
    {
        bool operator() (const String& a, const String& b) const
        {
            const uint32_t la = a.length();
            const uint32_t lb = b.length();
            if (la == lb)
                return (memcmp(a.ptr(), b.ptr(), la*sizeof(utf32)) < 0);
            return (la < lb);
        }
    };

public:
	String(void)
	{
		init();
	}
	~String(void);
	String(const String& str)
	{
		init();
		assign(str);
	}
	String(const String& str, size_type str_idx, size_type str_num = npos)
	{
		init();
		assign(str, str_idx, str_num);
	}
	String(const std::string& std_str)
	{
		init();
		assign(std_str);
	}
	String(const std::string& std_str, size_type str_idx, size_type str_num = npos)
	{
		init();
		assign(std_str, str_idx, str_num);
	}
	String(const utf8* utf8_str)
	{
		init();
		assign(utf8_str);
	}
	String(const utf8* utf8_str, size_type chars_len)
	{
		init();
		assign(utf8_str, chars_len);
	}
	String(size_type num, utf32 code_point)
	{
		init();
		assign(num, code_point);
	}
	String(const_iterator iter_beg, const_iterator iter_end)
	{
		init();
		append(iter_beg, iter_end);
	}
	String(const char* cstr)
	{
		init();
		assign(cstr);
	}
	String(const char* chars, size_type chars_len)
	{
		init();
		assign(chars, chars_len);
	}
	void fromUtf8(const std::string& utf8_str);
	//std::string toGBK();

	//大小
	size_type	size(void) const
	{
		return mCplength;
	}
	//长度
	size_type	length(void) const
	{
		return mCplength;
	}
	//是否为空
	bool	empty(void) const
	{
		return	(mCplength == 0);
	}
	//最大大小
	static size_type	max_size(void)
	{
		return (((size_type)-1) / sizeof(utf32));
	}
	//返回最大位置
	size_type capacity(void) const
	{
		return mReserve - 1;
	}
	//保留
	void	reserve(size_type num = 0)
	{
		if (num == 0)
			trim();
		else
			grow(num);
	}

	//比较
	int		compare(const String& str) const
	{
		return compare(0, mCplength, str);
	}

	int		compare(size_type idx, size_type len, const String& str, size_type str_idx = 0, size_type str_len = npos) const
	{
		if ((mCplength < idx) || (str.mCplength < str_idx))
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if ((len == npos) || (idx + len > mCplength))
			len = mCplength - idx;

		if ((str_len == npos) || (str_idx + str_len > str.mCplength))
			str_len = str.mCplength - str_idx;

		int val = (len == 0) ? 0 : utf32_comp_utf32(&ptr()[idx], &str.ptr()[str_idx], (len < str_len) ? len : str_len);

		return (val != 0) ? ((val < 0) ? -1 : 1) : (len < str_len) ? -1 : (len == str_len) ? 0 : 1;
	}

	int		compare(const std::string& std_str) const
	{
		return compare(0, mCplength, std_str);
	}


	int		compare(size_type idx, size_type len, const std::string& std_str, size_type str_idx = 0, size_type str_len = npos) const
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (std_str.size() < str_idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for string");

		if ((len == npos) || (idx + len > mCplength))
			len = mCplength - idx;

		if ((str_len == npos) || (str_idx + str_len > std_str.size()))
			str_len = (size_type)std_str.size() - str_idx;

		int val = (len == 0) ? 0 : utf32_comp_char(&ptr()[idx], &std_str.c_str()[str_idx], (len < str_len) ? len : str_len);

		return (val != 0) ? ((val < 0) ? -1 : 1) : (len < str_len) ? -1 : (len == str_len) ? 0 : 1;
	}

	int		compare(const utf8* utf8_str) const
	{
		return compare(0, mCplength, utf8_str, encoded_size(utf8_str));
	}

	int		compare(size_type idx, size_type len, const utf8* utf8_str) const
	{
		return compare(idx, len, utf8_str, encoded_size(utf8_str));
	}

	int		compare(size_type idx, size_type len, const utf8* utf8_str, size_type str_cplen) const
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (str_cplen == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for utf8 encoded string can not be 'npos'");

		if ((len == npos) || (idx + len > mCplength))
			len = mCplength - idx;

		int val = (len == 0) ? 0 : utf32_comp_utf8(&ptr()[idx], utf8_str, (len < str_cplen) ? len : str_cplen);

		return (val != 0) ? ((val < 0) ? -1 : 1) : (len < str_cplen) ? -1 : (len == str_cplen) ? 0 : 1;
	}

	int		compare(const char* cstr) const
	{
		return compare(0, mCplength, cstr, strlen(cstr));
	}

	int		compare(size_type idx, size_type len, const char* cstr) const
	{
		return compare(idx, len, cstr, strlen(cstr));
	}

	int		compare(size_type idx, size_type len, const char* chars, size_type chars_len) const
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (chars_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for char array can not be 'npos'");

		if ((len == npos) || (idx + len > mCplength))
			len = mCplength - idx;

		int val = (len == 0) ? 0 : utf32_comp_char(&ptr()[idx], chars, (len < chars_len) ? len : chars_len);

		return (val != 0) ? ((val < 0) ? -1 : 1) : (len < chars_len) ? -1 : (len == chars_len) ? 0 : 1;
	}
	//索引
	reference	operator[](size_type idx)
	{
		return (ptr()[idx]);
	}

	value_type	operator[](size_type idx) const
	{
		return ptr()[idx];
	}

	reference	at(size_type idx)
	{
		if (mCplength <= idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		return ptr()[idx];
	}

	const_reference	at(size_type idx) const
	{
		if (mCplength <= idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		return ptr()[idx];
	}
	//数据
	const char* c_str(void) const
	{
		return (const char*)build_utf8_buff();
	}
	//数据
	const utf8* data(void) const
	{
		return build_utf8_buff();
	}

	//指针
	utf32*	ptr(void)
	{
		return (mReserve > STR_QUICKBUFF_SIZE) ? mBuffer : mQuickbuff;
	}

	//指针
	const utf32*	ptr(void) const
	{
		return (mReserve > STR_QUICKBUFF_SIZE) ? mBuffer : mQuickbuff;
	}
	//复制
	size_type	copy(utf8* buf, size_type len = npos, size_type idx = 0) const
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (len == npos)
			len = mCplength;

		return encode(&ptr()[idx], buf, npos, len);
	}
	size_type	utf8_stream_len(size_type num = npos, size_type idx = 0) const
	{
		using namespace std;

		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index was out of range for String object");

		size_type	maxlen = mCplength - idx;

		return encoded_size(&ptr()[idx], std::min(num, maxlen));
	}
	String&	operator=(const String& str)
	{
		return assign(str);
	}
	String&	assign(const String& str, size_type str_idx = 0, size_type str_num = npos)
	{
		if (str.mCplength < str_idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index was out of range for String object");

		if ((str_num == npos) || (str_num > str.mCplength - str_idx))
			str_num = str.mCplength - str_idx;

		grow(str_num);
		setlen(str_num);
		memcpy(ptr(), &str.ptr()[str_idx], str_num * sizeof(utf32));

		return *this;
	}
	String&	operator=(const std::string& std_str)
	{
		return assign(std_str);
	}
	//分配
	String&	assign(const std::string& std_str, size_type str_idx = 0, size_type str_num = npos)
	{
		if (std_str.size() < str_idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index was out of range for string object");

		if ((str_num == npos) || (str_num > (size_type)std_str.size() - str_idx))
			str_num = (size_type)std_str.size() - str_idx;

		grow(str_num);
		setlen(str_num);

		while(str_num--)
		{
			((*this)[str_num]) = static_cast<utf32>(static_cast<unsigned char>(std_str[str_num + str_idx]));
		}

		return *this;
	}
	String&	operator=(const utf8* utf8_str)
	{
		return assign(utf8_str, utf_length(utf8_str));
	}

	String&	assign(const utf8* utf8_str)
	{
		return assign(utf8_str, utf_length(utf8_str));
	}

	String&	assign(const utf8* utf8_str, size_type str_num)
	{
		if (str_num == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for utf8 encoded string can not be 'npos'");

		size_type enc_sze = encoded_size(utf8_str, str_num);

		grow(enc_sze);
		encode(utf8_str, ptr(), mReserve, str_num);
		setlen(enc_sze);
		return *this;
	}

	String&	operator=(utf32 code_point)
	{
		return assign(1, code_point);
	}

	String&	assign(size_type num, utf32 code_point)
	{
		if (num == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Code point count can not be 'npos'");

		grow(num);
		setlen(num);
		utf32* p = ptr();

		while(num--)
			*p++ = code_point;

		return *this;
	}

	String&	operator=(const char* cstr)
	{
		return assign(cstr, strlen(cstr));
	}


	String&	assign(const char* cstr)
	{
		return assign(cstr, strlen(cstr));
	}


	String&	assign(const char* chars, size_type chars_len)
	{
		grow(chars_len);
		utf32* pt = ptr();

		for (size_type i = 0; i < chars_len; ++i)
		{
			*pt++ = static_cast<utf32>(static_cast<unsigned char>(*chars++));
		}

		setlen(chars_len);
		return *this;
	}

	//交换
	void	swap(String& str)
	{
		size_type	temp_len	= mCplength;
		mCplength = str.mCplength;
		str.mCplength = temp_len;

		size_type	temp_res	= mReserve;
		mReserve = str.mReserve;
		str.mReserve = temp_res;

		utf32*		temp_buf	= mBuffer;
		mBuffer = str.mBuffer;
		str.mBuffer = temp_buf;

		// see if we need to swap 'quick buffer' data
		if (temp_res <= STR_QUICKBUFF_SIZE)
		{
			utf32		temp_qbf[STR_QUICKBUFF_SIZE];

			memcpy(temp_qbf, mQuickbuff, STR_QUICKBUFF_SIZE * sizeof(utf32));
			memcpy(mQuickbuff, str.mQuickbuff, STR_QUICKBUFF_SIZE * sizeof(utf32));
			memcpy(str.mQuickbuff, temp_qbf, STR_QUICKBUFF_SIZE * sizeof(utf32));
		}

	}
	//追加
	String&	operator+=(const String& str)
	{
		return append(str);
	}

	String& append(const String& str, size_type str_idx = 0, size_type str_num = npos)
	{
		if (str.mCplength < str_idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if ((str_num == npos) || (str_num > str.mCplength - str_idx))
			str_num = str.mCplength - str_idx;

		grow(mCplength + str_num);
		memcpy(&ptr()[mCplength], &str.ptr()[str_idx], str_num * sizeof(utf32));
		setlen(mCplength + str_num);
		return *this;
	}

	String&	operator+=(const std::string& std_str)
	{
		return append(std_str);
	}

	String& append(const std::string& std_str, size_type str_idx = 0, size_type str_num = npos)
	{
		if (std_str.size() < str_idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for string");

		if ((str_num == npos) || (str_num > (size_type)std_str.size() - str_idx))
			str_num = (size_type)std_str.size() - str_idx;

		size_type newsze = mCplength + str_num;

		grow(newsze);
		utf32* pt = &ptr()[newsze-1];

		while(str_num--)
			*pt-- = static_cast<utf32>(static_cast<unsigned char>(std_str[str_num]));

		setlen(newsze);
		return *this;
	}

	String&	operator+=(const utf8* utf8_str)
	{
		return append(utf8_str, utf_length(utf8_str));
	}

	String& append(const utf8* utf8_str)
	{
		return append(utf8_str, utf_length(utf8_str));
	}

	String& append(const utf8* utf8_str, size_type len)
	{
		if (len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for utf8 encoded string can not be 'npos'");

		size_type encsz = encoded_size(utf8_str, len);
		size_type newsz = mCplength + encsz;

		grow(newsz);
		encode(utf8_str, &ptr()[mCplength], encsz, len);
		setlen(newsz);

		return *this;
	}


	String& operator+=(utf32 code_point)
	{
		return append(1, code_point);
	}

	String& append(size_type num, utf32 code_point)
	{
		if (num == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Code point count can not be 'npos'");

		size_type newsz = mCplength + num;
		grow(newsz);

		utf32* p = &ptr()[mCplength];

		while(num--)
			*p++ = code_point;

		setlen(newsz);

		return *this;
	}

	void	push_back(utf32 code_point)
	{
		append(1, code_point);
	}

	String&	append(const_iterator iter_beg, const_iterator iter_end)
	{
		return replace(end(), end(), iter_beg, iter_end);
	}


	String&	operator+=(const char* cstr)
	{
		return append(cstr, strlen(cstr));
	}

	String& append(const char* cstr)
	{
		return append(cstr, strlen(cstr));
	}

	String& append(const char* chars, size_type chars_len)
	{
		if (chars_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for char array can not be 'npos'");

		size_type newsz = mCplength + chars_len;

		grow(newsz);

		utf32* pt = &ptr()[newsz-1];

		while(chars_len--)
			*pt-- = static_cast<utf32>(static_cast<unsigned char>(chars[chars_len]));

		setlen(newsz);

		return *this;
	}

	//插入
	String&	insert(size_type idx, const String& str)
	{
		return insert(idx, str, 0, npos);
	}

	String& insert(size_type idx, const String& str, size_type str_idx, size_type str_num)
	{
		if ((mCplength < idx) || (str.mCplength < str_idx))
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if ((str_num == npos) || (str_num > str.mCplength - str_idx))
			str_num = str.mCplength - str_idx;

		size_type newsz = mCplength + str_num;
		grow(newsz);
		memmove(&ptr()[idx + str_num], &ptr()[idx], (mCplength - idx) * sizeof(utf32));
		memcpy(&ptr()[idx], &str.ptr()[str_idx], str_num * sizeof(utf32));
		setlen(newsz);

		return *this;
	}

	String&	insert(size_type idx, const std::string& std_str)
	{
		return insert(idx, std_str, 0, npos);
	}

	String& insert(size_type idx, const std::string& std_str, size_type str_idx, size_type str_num)
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (std_str.size() < str_idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for string");

		if ((str_num == npos) || (str_num > (size_type)std_str.size() - str_idx))
			str_num = (size_type)std_str.size() - str_idx;

		size_type newsz = mCplength + str_num;
		grow(newsz);

		memmove(&ptr()[idx + str_num], &ptr()[idx], (mCplength - idx) * sizeof(utf32));

		utf32* pt = &ptr()[idx + str_num - 1];

		while(str_num--)
			*pt-- = static_cast<utf32>(static_cast<unsigned char>(std_str[str_idx + str_num]));

		setlen(newsz);

		return *this;
	}

	String&	insert(size_type idx, const utf8* utf8_str)
	{
		return insert(idx, utf8_str, utf_length(utf8_str));
	}

	String& insert(size_type idx, const utf8* utf8_str, size_type len)
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length of utf8 encoded string can not be 'npos'");

		size_type encsz = encoded_size(utf8_str, len);
		size_type newsz = mCplength + encsz;

		grow(newsz);
		memmove(&ptr()[idx + encsz], &ptr()[idx], (mCplength - idx) * sizeof(utf32));
		encode(utf8_str, &ptr()[idx], encsz, len);
		setlen(newsz);

		return *this;
	}

	String& insert(size_type idx, size_type num, utf32 code_point)
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (num == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Code point count can not be 'npos'");

		size_type newsz = mCplength + num;
		grow(newsz);

		memmove(&ptr()[idx + num], &ptr()[idx], (mCplength - idx) * sizeof(utf32));

		utf32* pt = &ptr()[idx + num - 1];

		while(num--)
			*pt-- = code_point;

		setlen(newsz);

		return *this;
	}

	void insert(iterator pos, size_type num, utf32 code_point)
	{
		insert(safe_iter_dif(pos, begin()), num, code_point);
	}

	iterator insert(iterator pos, utf32 code_point)
	{
		insert(pos, 1, code_point);
		return pos;
	}

	void	insert(iterator iter_pos, const_iterator iter_beg, const_iterator iter_end)
	{
		replace(iter_pos, iter_pos, iter_beg, iter_end);
	}
	String&	insert(size_type idx, const char* cstr)
	{
		return insert(idx, cstr, strlen(cstr));
	}
	String& insert(size_type idx, const char* chars, size_type chars_len)
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (chars_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length of char array can not be 'npos'");

		size_type newsz = mCplength + chars_len;

		grow(newsz);
		memmove(&ptr()[idx + chars_len], &ptr()[idx], (mCplength - idx) * sizeof(utf32));

		utf32* pt = &ptr()[idx + chars_len - 1];

		while(chars_len--)
			*pt-- = static_cast<utf32>(static_cast<unsigned char>(chars[chars_len]));

		setlen(newsz);

		return *this;
	}

	//移除
	void	clear(void)
	{
		setlen(0);
		trim();
	}
	String& erase(void)
	{
		clear();
		return *this;
	}
	String&	erase(size_type idx)
	{
		return erase(idx, 1);
	}
	String& erase(size_type idx, size_type len)
	{
        // cover the no-op case.
        if (len == 0)
            return *this;

		if (mCplength <= idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (len == npos)
			len = mCplength - idx;

		size_type newsz = mCplength - len;

		memmove(&ptr()[idx], &ptr()[idx + len], (mCplength - idx - len) * sizeof(utf32));
		setlen(newsz);
		return	*this;
	}
	String& erase(iterator pos)
	{
		return erase(safe_iter_dif(pos, begin()), 1);
	}

	String& erase(iterator iter_beg, iterator iter_end)
	{
		return erase(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg));
	}
	//重置大小
	void	resize(size_type num)
	{
		resize(num, utf32());
	}

	void	resize(size_type num, utf32 code_point)
	{
		if (num < mCplength)
		{
			setlen(num);
		}
		else
		{
			append(num - mCplength, code_point);
		}

	}
	//替换
	String& replace(size_type idx, size_type len, const String& str)
	{
		return replace(idx, len, str, 0, npos);
	}

	String& replace(iterator iter_beg, iterator iter_end, const String& str)
	{
		return replace(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg), str, 0, npos);
	}

	String& replace(size_type idx, size_type len, const String& str, size_type str_idx, size_type str_num)
	{
		if ((mCplength < idx) || (str.mCplength < str_idx))
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (((str_idx + str_num) > str.mCplength) || (str_num == npos))
			str_num = str.mCplength - str_idx;

		if (((len + idx) > mCplength) || (len == npos))
			len = mCplength - idx;

		size_type newsz = mCplength + str_num - len;

		grow(newsz);

		if ((idx + len) < mCplength)
			memmove(&ptr()[idx + str_num], &ptr()[len + idx], (mCplength - idx - len) * sizeof(utf32));

		memcpy(&ptr()[idx], &str.ptr()[str_idx], str_num * sizeof(utf32));
		setlen(newsz);

		return *this;
	}


	String& replace(size_type idx, size_type len, const std::string& std_str)
	{
		return replace(idx, len, std_str, 0, npos);
	}
	String& replace(iterator iter_beg, iterator iter_end, const std::string& std_str)
	{
		return replace(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg), std_str, 0, npos);
	}
	String& replace(size_type idx, size_type len, const std::string& std_str, size_type str_idx, size_type str_num)
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (std_str.size() < str_idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for string");

		if (((str_idx + str_num) > std_str.size()) || (str_num == npos))
			str_num = (size_type)std_str.size() - str_idx;

		if (((len + idx) > mCplength) || (len == npos))
			len = mCplength - idx;

		size_type newsz = mCplength + str_num - len;

		grow(newsz);

		if ((idx + len) < mCplength)
			memmove(&ptr()[idx + str_num], &ptr()[len + idx], (mCplength - idx - len) * sizeof(utf32));

		utf32* pt = &ptr()[idx + str_num - 1];

		while (str_num--)
			*pt-- = static_cast<utf32>(static_cast<unsigned char>(std_str[str_idx + str_num]));

		setlen(newsz);

		return *this;
	}

	String& replace(size_type idx, size_type len, const utf8* utf8_str)
	{
		return replace(idx, len, utf8_str, utf_length(utf8_str));
	}

	String& replace(iterator iter_beg, iterator iter_end, const utf8* utf8_str)
	{
		return replace(iter_beg, iter_end, utf8_str, utf_length(utf8_str));
	}

	String& replace(size_type idx, size_type len, const utf8* utf8_str, size_type str_len)
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (str_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for utf8 encoded string can not be 'npos'");

		if (((len + idx) > mCplength) || (len == npos))
			len = mCplength - idx;

		size_type encsz = encoded_size(utf8_str, str_len);
		size_type newsz = mCplength + encsz - len;

		grow(newsz);

		if ((idx + len) < mCplength)
			memmove(&ptr()[idx + encsz], &ptr()[len + idx], (mCplength - idx - len) * sizeof(utf32));

		encode(utf8_str, &ptr()[idx], encsz, str_len);

		setlen(newsz);
		return *this;
	}

	String& replace(iterator iter_beg, iterator iter_end, const utf8* utf8_str, size_type str_len)
	{
		return replace(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg), utf8_str, str_len);
	}

	String& replace(size_type idx, size_type len, size_type num, utf32 code_point)
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (num == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Code point count can not be 'npos'");

		if (((len + idx) > mCplength) || (len == npos))
			len = mCplength - idx;

		size_type newsz = mCplength + num - len;

		grow(newsz);

		if ((idx + len) < mCplength)
			memmove(&ptr()[idx + num], &ptr()[len + idx], (mCplength - idx - len) * sizeof(utf32));

		utf32* pt = &ptr()[idx + num - 1];

		while (num--)
			*pt-- = code_point;

		setlen(newsz);

		return *this;
	}

	String& replace(iterator iter_beg, iterator iter_end, size_type num, utf32 code_point)
	{
		return replace(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg), num, code_point);
	}

	String& replace(iterator iter_beg, iterator iter_end, const_iterator iter_newBeg, const_iterator iter_newEnd)
	{
		if (iter_newBeg == iter_newEnd)
		{
			erase(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg));
		}
		else
		{
			size_type str_len = safe_iter_dif(iter_newEnd, iter_newBeg);
			size_type idx = safe_iter_dif(iter_beg, begin());
			size_type len = safe_iter_dif(iter_end, iter_beg);

			if ((len + idx) > mCplength)
				len = mCplength - idx;

			size_type newsz = mCplength + str_len - len;

			grow(newsz);

			if ((idx + len) < mCplength)
				memmove(&ptr()[idx + str_len], &ptr()[len + idx], (mCplength - idx - len) * sizeof(utf32));

			memcpy(&ptr()[idx], iter_newBeg.mPtr, str_len * sizeof(utf32));
			setlen(newsz);
		}

		return *this;
	}

	String& replace(size_type idx, size_type len, const char* cstr)
	{
		return replace(idx, len, cstr, strlen(cstr));
	}

	String& replace(iterator iter_beg, iterator iter_end, const char* cstr)
	{
		return replace(iter_beg, iter_end, cstr, strlen(cstr));
	}

	String& replace(size_type idx, size_type len, const char* chars, size_type chars_len)
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for String");

		if (chars_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for the char array can not be 'npos'");

		if (((len + idx) > mCplength) || (len == npos))
			len = mCplength - idx;

		size_type newsz = mCplength + chars_len - len;

		grow(newsz);

		if ((idx + len) < mCplength)
			memmove(&ptr()[idx + chars_len], &ptr()[len + idx], (mCplength - idx - len) * sizeof(utf32));

		utf32* pt = &ptr()[idx + chars_len - 1];

		while (chars_len--)
			*pt-- = static_cast<utf32>(static_cast<unsigned char>(chars[chars_len]));

		setlen(newsz);
		return *this;
	}

	String& replace(iterator iter_beg, iterator iter_end, const char* chars, size_type chars_len)
	{
		return replace(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg), chars, chars_len);
	}
	//查找
	size_type	find(utf32 code_point, size_type idx = 0) const
	{
		if (idx < mCplength)
		{
			const utf32* pt = &ptr()[idx];

			while (idx < mCplength)
			{
				if (*pt++ == code_point)
					return idx;

				++idx;
			}

		}

		return npos;
	}

	size_type	rfind(utf32 code_point, size_type idx = npos) const
	{
		if (idx >= mCplength)
			idx = mCplength - 1;

		if (mCplength > 0)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (*pt-- == code_point)
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}
	size_type	find(const String& str, size_type idx = 0) const
	{
		if ((str.mCplength == 0) && (idx < mCplength))
			return idx;

		if (idx < mCplength)
		{
			// loop while search string could fit in to search area
			while (mCplength - idx >= str.mCplength)
			{
				if (0 == compare(idx, str.mCplength, str))
					return idx;

				++idx;
			}

		}

		return npos;
	}
	size_type	rfind(const String& str, size_type idx = npos) const
	{
		if (str.mCplength == 0)
			return (idx < mCplength) ? idx : mCplength;

		if (str.mCplength <= mCplength)
		{
			if (idx > (mCplength - str.mCplength))
				idx = mCplength - str.mCplength;

			do
			{
				if (0 == compare(idx, str.mCplength, str))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	size_type	find(const std::string& std_str, size_type idx = 0) const
	{
		std::string::size_type sze = std_str.size();

		if ((sze == 0) && (idx < mCplength))
			return idx;

		if (idx < mCplength)
		{
			// loop while search string could fit in to search area
			while (mCplength - idx >= sze)
			{
				if (0 == compare(idx, (size_type)sze, std_str))
					return idx;

				++idx;
			}

		}

		return npos;
	}

	size_type	rfind(const std::string& std_str, size_type idx = npos) const
	{
		std::string::size_type sze = std_str.size();

		if (sze == 0)
			return (idx < mCplength) ? idx : mCplength;

		if (sze <= mCplength)
		{
			if (idx > (mCplength - sze))
				idx = mCplength - sze;

			do
			{
				if (0 == compare(idx, (size_type)sze, std_str))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	size_type	find(const utf8* utf8_str, size_type idx = 0) const
	{
		return find(utf8_str, idx, utf_length(utf8_str));
	}

	size_type	rfind(const utf8* utf8_str, size_type idx = npos) const
	{
		return rfind(utf8_str, idx, utf_length(utf8_str));
	}

	size_type	find(const utf8* utf8_str, size_type idx, size_type str_len) const
	{
		if (str_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for utf8 encoded string can not be 'npos'");

		size_type sze = encoded_size(utf8_str, str_len);

		if ((sze == 0) && (idx < mCplength))
			return idx;

		if (idx < mCplength)
		{
			// loop while search string could fit in to search area
			while (mCplength - idx >= sze)
			{
				if (0 == compare(idx, sze, utf8_str, sze))
					return idx;

				++idx;
			}

		}

		return npos;
	}

	size_type	rfind(const utf8* utf8_str, size_type idx, size_type str_len) const
	{
		if (str_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for utf8 encoded string can not be 'npos'");

		size_type sze = encoded_size(utf8_str, str_len);

		if (sze == 0)
			return (idx < mCplength) ? idx : mCplength;

		if (sze <= mCplength)
		{
			if (idx > (mCplength - sze))
				idx = mCplength - sze;

			do
			{
				if (0 == compare(idx, sze, utf8_str, sze))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}


	size_type	find(const char* cstr, size_type idx = 0) const
	{
		return find(cstr, idx, strlen(cstr));
	}
	size_type	rfind(const char* cstr, size_type idx = npos) const
	{
		return rfind(cstr, idx, strlen(cstr));
	}


	size_type	find(const char* chars, size_type idx, size_type chars_len) const
	{
		if (chars_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for char array can not be 'npos'");

		if ((chars_len == 0) && (idx < mCplength))
			return idx;

		if (idx < mCplength)
		{
			// loop while search string could fit in to search area
			while (mCplength - idx >= chars_len)
			{
				if (0 == compare(idx, chars_len, chars, chars_len))
					return idx;

				++idx;
			}

		}

		return npos;
	}

	size_type	rfind(const char* chars, size_type idx, size_type chars_len) const
	{
		if (chars_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for char array can not be 'npos'");

		if (chars_len == 0)
			return (idx < mCplength) ? idx : mCplength;

		if (chars_len <= mCplength)
		{
			if (idx > (mCplength - chars_len))
				idx = mCplength - chars_len;

			do
			{
				if (0 == compare(idx, chars_len, chars, chars_len))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}
	size_type	find_first_of(const String& str, size_type idx = 0) const
	{
		if (idx < mCplength)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != str.find(*pt++))
					return idx;

			} while (++idx != mCplength);

		}

		return npos;
	}
	size_type	find_first_not_of(const String& str, size_type idx = 0) const
	{
		if (idx < mCplength)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == str.find(*pt++))
					return idx;

			} while (++idx != mCplength);

		}

		return npos;
	}

	size_type	find_first_of(const std::string& std_str, size_type idx = 0) const
	{
		if (idx < mCplength)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != find_codepoint(std_str, *pt++))
					return idx;

			} while (++idx != mCplength);

		}

		return npos;
	}

	size_type	find_first_not_of(const std::string& std_str, size_type idx = 0) const
	{
		if (idx < mCplength)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == find_codepoint(std_str, *pt++))
					return idx;

			} while (++idx != mCplength);

		}

		return npos;
	}

	size_type	find_first_of(const utf8* utf8_str, size_type idx = 0) const
	{
		return find_first_of(utf8_str, idx, utf_length(utf8_str));
	}
	size_type	find_first_not_of(const utf8* utf8_str, size_type idx = 0) const
	{
		return find_first_not_of(utf8_str, idx, utf_length(utf8_str));
	}

	size_type	find_first_of(const utf8* utf8_str, size_type idx, size_type str_len) const
	{
		if (str_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for utf8 encoded string can not be 'npos'");

		if (idx < mCplength)
		{
			size_type encsze = encoded_size(utf8_str, str_len);

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != find_codepoint(utf8_str, encsze, *pt++))
					return idx;

			} while (++idx != mCplength);

		}

		return npos;
	}
	size_type	find_first_not_of(const utf8* utf8_str, size_type idx, size_type str_len) const
	{
		if (str_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for utf8 encoded string can not be 'npos'");

		if (idx < mCplength)
		{
			size_type encsze = encoded_size(utf8_str, str_len);

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == find_codepoint(utf8_str, encsze, *pt++))
					return idx;

			} while (++idx != mCplength);

		}

		return npos;
	}

	size_type	find_first_of(utf32 code_point, size_type idx = 0) const
	{
		return find(code_point, idx);
	}

	size_type	find_first_not_of(utf32 code_point, size_type idx = 0) const
	{
		if (idx < mCplength)
		{
			do
			{
				if ((*this)[idx] != code_point)
					return idx;

			} while(idx++ < mCplength);

		}

		return npos;
	}

	size_type	find_first_of(const char* cstr, size_type idx = 0) const
	{
		return find_first_of(cstr, idx, strlen(cstr));
	}

	size_type	find_first_not_of(const char* cstr, size_type idx = 0) const
	{
		return find_first_not_of(cstr, idx, strlen(cstr));
	}

	size_type	find_first_of(const char* chars, size_type idx, size_type chars_len) const
	{
		if (chars_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for char array can not be 'npos'");

		if (idx < mCplength)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != find_codepoint(chars, chars_len, *pt++))
					return idx;

			} while (++idx != mCplength);

		}

		return npos;
	}

	size_type	find_first_not_of(const char* chars, size_type idx, size_type chars_len) const
	{
		if (chars_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for char array can not be 'npos'");

		if (idx < mCplength)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == find_codepoint(chars, chars_len, *pt++))
					return idx;

			} while (++idx != mCplength);

		}

		return npos;
	}

	size_type	find_last_of(const String& str, size_type idx = npos) const
	{
		if (mCplength > 0)
		{
			if (idx >= mCplength)
				idx = mCplength - 1;

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != str.find(*pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	size_type	find_last_not_of(const String& str, size_type idx = npos) const
	{
		if (mCplength > 0)
		{
			if (idx >= mCplength)
				idx = mCplength - 1;

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == str.find(*pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}


	size_type	find_last_of(const std::string& std_str, size_type idx = npos) const
	{
		if (mCplength > 0)
		{
			if (idx >= mCplength)
				idx = mCplength - 1;

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != find_codepoint(std_str, *pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	size_type	find_last_not_of(const std::string& std_str, size_type idx = npos) const
	{
		if (mCplength > 0)
		{
			if (idx >= mCplength)
				idx = mCplength - 1;

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == find_codepoint(std_str, *pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	size_type	find_last_of(const utf8* utf8_str, size_type idx = npos) const
	{
		return find_last_of(utf8_str, idx, utf_length(utf8_str));
	}
	size_type	find_last_not_of(const utf8* utf8_str, size_type idx = npos) const
	{
		return find_last_not_of(utf8_str, idx, utf_length(utf8_str));
	}

	size_type	find_last_of(const utf8* utf8_str, size_type idx, size_type str_len) const
	{
		if (str_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for utf8 encoded string can not be 'npos'");

		if (mCplength > 0)
		{
			if (idx >= mCplength)
				idx = mCplength - 1;

			size_type encsze = encoded_size(utf8_str, str_len);

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != find_codepoint(utf8_str, encsze, *pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	size_type	find_last_not_of(const utf8* utf8_str, size_type idx, size_type str_len) const
	{
		if (str_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for utf8 encoded string can not be 'npos'");

		if (mCplength > 0)
		{
			if (idx >= mCplength)
				idx = mCplength - 1;

			size_type encsze = encoded_size(utf8_str, str_len);

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == find_codepoint(utf8_str, encsze, *pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	size_type	find_last_of(utf32 code_point, size_type idx = npos) const
	{
		return rfind(code_point, idx);
	}

	size_type	find_last_not_of(utf32 code_point, size_type idx = npos) const
	{
		if (mCplength > 0)
		{
			if (idx >= mCplength)
				idx = mCplength - 1;

			do
			{
				if ((*this)[idx] != code_point)
					return idx;

			} while(idx-- != 0);

		}

		return npos;
	}
	size_type	find_last_of(const char* cstr, size_type idx = npos) const
	{
		return find_last_of(cstr, idx, strlen(cstr));
	}

	size_type	find_last_not_of(const char* cstr, size_type idx = npos) const
	{
		return find_last_not_of(cstr, idx, strlen(cstr));
	}

	size_type	find_last_of(const char* chars, size_type idx, size_type chars_len) const
	{
		if (chars_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for char array can not be 'npos'");

		if (mCplength > 0)
		{
			if (idx >= mCplength)
				idx = mCplength - 1;

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != find_codepoint(chars, chars_len, *pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	size_type	find_last_not_of(const char* chars, size_type idx, size_type chars_len) const
	{
		if (chars_len == npos)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Length for char array can not be 'npos'");

		if (mCplength > 0)
		{
			if (idx >= mCplength)
				idx = mCplength - 1;

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == find_codepoint(chars, chars_len, *pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	//截取
	String	substr(size_type idx = 0, size_type len = npos) const
	{
		if (mCplength < idx)
			STRING_THROW_EXCEPT(Exception::ERR_INTERNAL_ERROR,
				"Index is out of range for this String");

		return String(*this, idx, len);
	}
	//迭代器
	iterator		begin(void)
	{
		return iterator(ptr());
	}

	const_iterator	begin(void) const
	{
		return const_iterator(ptr());
	}
	iterator		end(void)
	{
		return iterator(&ptr()[mCplength]);
	}
	const_iterator	end(void) const
	{
		return const_iterator(&ptr()[mCplength]);
	}

	reverse_iterator		rbegin(void)
	{
		return reverse_iterator(end());
	}

	const_reverse_iterator	rbegin(void) const
	{
		return const_reverse_iterator(end());
	}

	reverse_iterator		rend(void)
	{
		return reverse_iterator(begin());
	}

	const_reverse_iterator	rend(void) const
	{
		return const_reverse_iterator(begin());
	}

private:
	//实现函数
    bool	grow(size_type new_size);
	//移除
    void	trim(void);

	void	setlen(size_type len)
	{
		mCplength = len;
		ptr()[len] = (utf32)(0);
	}

	void	init(void)
	{
		mReserve			= STR_QUICKBUFF_SIZE;
		mEncodedbuff		= 0;
		mEncodedbufflen	= 0;
		mEncodeddatlen		= 0;
        mBuffer            = 0;
		setlen(0);
	}

	bool	inside(utf32* inptr)
	{
		if (inptr < ptr() || ptr() + mCplength <= inptr)
			return false;
		else
			return true;
	}

	size_type safe_iter_dif(const const_iterator& iter1, const const_iterator& iter2) const
	{
		return (iter1.mPtr == 0) ? 0 : (iter1 - iter2);
	}
	//编码
	size_type encode(const utf32* src, utf8* dest, size_type dest_len, size_type src_len = 0) const
	{
		if (src_len == 0)
		{
			src_len = utf_length(src);
		}

		size_type destCapacity = dest_len;

		for (uint idx = 0; idx < src_len; ++idx)
		{
			utf32	cp = src[idx];

			if (destCapacity < encoded_size(cp))
			{
				break;
			}

			if (cp < 0x80)
			{
				*dest++ = (utf8)cp;
				--destCapacity;
			}
			else if (cp < 0x0800)
			{
				*dest++ = (utf8)((cp >> 6) | 0xC0);
				*dest++ = (utf8)((cp & 0x3F) | 0x80);
				destCapacity -= 2;
			}
			else if (cp < 0x10000)
			{
				*dest++ = (utf8)((cp >> 12) | 0xE0);
				*dest++ = (utf8)(((cp >> 6) & 0x3F) | 0x80);
				*dest++ = (utf8)((cp & 0x3F) | 0x80);
				destCapacity -= 3;
			}
			else
			{
				*dest++ = (utf8)((cp >> 18) | 0xF0);
				*dest++ = (utf8)(((cp >> 12) & 0x3F) | 0x80);
				*dest++ = (utf8)(((cp >> 6) & 0x3F) | 0x80);
				*dest++ = (utf8)((cp & 0x3F) | 0x80);
				destCapacity -= 4;
			}

		}

		return dest_len - destCapacity;
	}

	size_type encode(const utf8* src, utf32* dest, size_type dest_len, size_type src_len = 0) const
	{
		if (src_len == 0)
		{
			src_len = utf_length(src);
		}

		size_type destCapacity = dest_len;

		for (uint idx = 0; ((idx < src_len) && (destCapacity > 0));)
		{
			utf32	cp;
			utf8	cu = src[idx++];

			if (cu < 0x80)
			{
				cp = (utf32)(cu);
			}
			else if (cu < 0xE0)
			{
				cp = ((cu & 0x1F) << 6);
				cp |= (src[idx++] & 0x3F);
			}
			else if (cu < 0xF0)
			{
				cp = ((cu & 0x0F) << 12);
				cp |= ((src[idx++] & 0x3F) << 6);
				cp |= (src[idx++] & 0x3F);
			}
			else
			{
				cp = ((cu & 0x07) << 18);
				cp |= ((src[idx++] & 0x3F) << 12);
				cp |= ((src[idx++] & 0x3F) << 6);
				cp |= (src[idx++] & 0x3F);
			}

			*dest++ = cp;
			--destCapacity;
		}

		return dest_len - destCapacity;
	}

	size_type encoded_size(utf32 code_point) const
	{
		if (code_point < 0x80)
			return 1;
		else if (code_point < 0x0800)
			return 2;
		else if (code_point < 0x10000)
			return 3;
		else
			return 4;
	}

	size_type encoded_size(const utf32* buf) const
	{
		return encoded_size(buf, utf_length(buf));
	}

	size_type encoded_size(const utf32* buf, size_type len) const
	{
		size_type count = 0;

		while (len--)
		{
			count += encoded_size(*buf++);
		}

		return count;
	}

	size_type encoded_size(const utf8* buf) const
	{
		return encoded_size(buf, utf_length(buf));
	}

	size_type encoded_size(const utf8* buf, size_type len) const
	{
		utf8 tcp;
		size_type count = 0;

		while (len--)
		{
			tcp = *buf++;
			++count;
			size_type size = 0;

			if (tcp < 0x80)
			{
			}
			else if (tcp < 0xE0)
			{
				size = 1;
				++buf;
			}
			else if (tcp < 0xF0)
			{
				size = 2;
				buf += 2;
			}
			else
			{
				size = 3;
				buf += 3;
			}

			if (len >= size)
				len -= size;
			else 
				break;
		}

		return count;
	}

	size_type utf_length(const utf8* utf8_str) const
	{
		size_type cnt = 0;
		while (*utf8_str++)
			cnt++;

		return cnt;
	}

	size_type utf_length(const utf32* utf32_str) const
	{
		size_type cnt = 0;
		while (*utf32_str++)
			cnt++;

		return cnt;
	}

    utf8* build_utf8_buff(void) const;

	int	utf32_comp_utf32(const utf32* buf1, const utf32* buf2, size_type cp_count) const
	{
		if (!cp_count)
			return 0;

		while ((--cp_count) && (*buf1 == *buf2))
			buf1++, buf2++;

		return *buf1 - *buf2;
	}

	int utf32_comp_char(const utf32* buf1, const char* buf2, size_type cp_count) const
	{
		if (!cp_count)
			return 0;

		while ((--cp_count) && (*buf1 == static_cast<utf32>(static_cast<unsigned char>(*buf2))))
			buf1++, buf2++;

		return *buf1 - static_cast<utf32>(static_cast<unsigned char>(*buf2));
	}

	int utf32_comp_utf8(const utf32* buf1, const utf8* buf2, size_type cp_count) const
	{
		if (!cp_count)
			return 0;

		utf32	cp;
		utf8	cu;

		do
		{
			cu = *buf2++;

			if (cu < 0x80)
			{
				cp = (utf32)(cu);
			}
			else if (cu < 0xE0)
			{
				cp = ((cu & 0x1F) << 6);
				cp |= (*buf2++ & 0x3F);
			}
			else if (cu < 0xF0)
			{
				cp = ((cu & 0x0F) << 12);
				cp |= ((*buf2++ & 0x3F) << 6);
				cp |= (*buf2++ & 0x3F);
			}
			else
			{
				cp = ((cu & 0x07) << 18);
				cp |= ((*buf2++ & 0x3F) << 12);
				cp |= ((*buf2++ & 0x3F) << 6);
				cp |= (*buf2++ & 0x3F);
			}

		} while ((*buf1++ == cp) && (--cp_count));

		return (*--buf1) - cp;
	}

	size_type find_codepoint(const std::string& str, utf32 code_point) const
	{
		size_type idx = 0, sze = (size_type)str.size();

		while (idx != sze)
		{
			if (code_point == static_cast<utf32>(static_cast<unsigned char>(str[idx])))
				return idx;

			++idx;
		}

		return npos;
	}

	size_type find_codepoint(const utf8* str, size_type len, utf32 code_point) const
	{
		size_type idx = 0;

		utf32	cp;
		utf8	cu;

		while (idx != len) {
			cu = *str++;

			if (cu < 0x80)
			{
				cp = (utf32)(cu);
			}
			else if (cu < 0xE0)
			{
				cp = ((cu & 0x1F) << 6);
				cp |= (*str++ & 0x3F);
			}
			else if (cu < 0xF0)
			{
				cp = ((cu & 0x0F) << 12);
				cp |= ((*str++ & 0x3F) << 6);
				cp |= (*str++ & 0x3F);
			}
			else
			{
				cp = ((cu & 0x07) << 18);
				cp |= ((*str++ & 0x3F) << 12);
				cp |= ((*str++ & 0x3F) << 6);
				cp |= (*str++ & 0x3F);
			}

			if (code_point == cp)
				return idx;

			++idx;
		}

		return npos;
	}


	size_type find_codepoint(const char* chars, size_type chars_len, utf32 code_point) const
	{
		for (size_type idx = 0; idx != chars_len; ++idx)
		{
			if (code_point == static_cast<utf32>(static_cast<unsigned char>(chars[idx])))
				return idx;
		}

		return npos;
	}

};


//比较运算符
bool JENGINE_API	operator==(const String& str1, const String& str2);
bool JENGINE_API	operator==(const String& str, const std::string& std_str);
bool JENGINE_API	operator==(const std::string& std_str, const String& str);
bool JENGINE_API	operator==(const String& str, const utf8* utf8_str);
bool JENGINE_API	operator==(const utf8* utf8_str, const String& str);
bool JENGINE_API	operator!=(const String& str1, const String& str2);
bool JENGINE_API	operator!=(const String& str, const std::string& std_str);
bool JENGINE_API	operator!=(const std::string& std_str, const String& str);
bool JENGINE_API	operator!=(const String& str, const utf8* utf8_str);
bool JENGINE_API	operator!=(const utf8* utf8_str, const String& str);
bool JENGINE_API	operator<(const String& str1, const String& str2);
bool JENGINE_API	operator<(const String& str, const std::string& std_str);
bool JENGINE_API	operator<(const std::string& std_str, const String& str);
bool JENGINE_API	operator<(const String& str, const utf8* utf8_str);
bool JENGINE_API	operator<(const utf8* utf8_str, const String& str);
bool JENGINE_API	operator>(const String& str1, const String& str2);
bool JENGINE_API	operator>(const String& str, const std::string& std_str);
bool JENGINE_API	operator>(const std::string& std_str, const String& str);
bool JENGINE_API	operator>(const String& str, const utf8* utf8_str);
bool JENGINE_API	operator>(const utf8* utf8_str, const String& str);
bool JENGINE_API	operator<=(const String& str1, const String& str2);
bool JENGINE_API	operator<=(const String& str, const std::string& std_str);
bool JENGINE_API	operator<=(const std::string& std_str, const String& str);
bool JENGINE_API	operator<=(const String& str, const utf8* utf8_str);
bool JENGINE_API	operator<=(const utf8* utf8_str, const String& str);
bool JENGINE_API	operator>=(const String& str1, const String& str2);
bool JENGINE_API	operator>=(const String& str, const std::string& std_str);
bool JENGINE_API	operator>=(const std::string& std_str, const String& str);
bool JENGINE_API	operator>=(const String& str, const utf8* utf8_str);
bool JENGINE_API	operator>=(const utf8* utf8_str, const String& str);
bool JENGINE_API	operator==(const String& str, const char* c_str);
bool JENGINE_API	operator==(const char* c_str, const String& str);
bool JENGINE_API	operator!=(const String& str, const char* c_str);
bool JENGINE_API	operator!=(const char* c_str, const String& str);
bool JENGINE_API	operator<(const String& str, const char* c_str);
bool JENGINE_API	operator<(const char* c_str, const String& str);
bool JENGINE_API	operator>(const String& str, const char* c_str);
bool JENGINE_API	operator>(const char* c_str, const String& str);
bool JENGINE_API	operator<=(const String& str, const char* c_str);
bool JENGINE_API	operator<=(const char* c_str, const String& str);
bool JENGINE_API	operator>=(const String& str, const char* c_str);
bool JENGINE_API	operator>=(const char* c_str, const String& str);
//串联
String JENGINE_API	operator+(const String& str1, const String& str2);
String JENGINE_API	operator+(const String& str, const std::string& std_str);
String JENGINE_API	operator+(const std::string& std_str, const String& str);
String JENGINE_API	operator+(const String& str, const utf8* utf8_str);
String JENGINE_API	operator+(const utf8* utf8_str, const String& str);
String JENGINE_API	operator+(const String& str, utf32 code_point);
String JENGINE_API	operator+(utf32 code_point, const String& str);
String JENGINE_API	operator+(const String& str, const char* c_str);
String JENGINE_API	operator+(const char* c_str, const String& str);
//输出
JENGINE_API std::ostream& operator<<(std::ostream& s, const String& str);
//交换
void JENGINE_API swap(String& str1, String& str2);



#endif	// _STRING_H_734083D4_72FA_43E9_A697_DEEA47F4CA97
