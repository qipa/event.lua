/*
\brief 文件描述需要修改
\file WordFilterUtil.h

Date:2017年5月23日
Author:揭育龙

Copyright (c) 2017 创娱游戏. All rights reserved.
*/
#ifndef _WORDFILTERUTIL_H
#define _WORDFILTERUTIL_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include "String.h"

//敏感字过滤器
class WordFilterUtil
{
public:
	WordFilterUtil();
	~WordFilterUtil();
	//过滤敏感字
	std::string filter(const std::string& input, int& replaceCount);
	//添加一个关键字
	void addKeywords(const std::string& keywords);
public:
	//第一个为长度信息，第二个为开始字符
	typedef std::pair<unsigned char, utf32> LengthInfo;
	typedef std::vector<LengthInfo> LengthInfoList;
	//第一个为结尾字符，第二个为长度信息，长度信息需要保证是降序排序
	typedef std::map<utf32, LengthInfoList> WordMap;
	//敏感字列表，第一个为敏感字，第二个无意义
	typedef std::set<String, String::FastLessCompare> KeywordsMap;

protected:
	WordMap				mWordMap;
	KeywordsMap			mKeywordsMap;
};


#endif //_WORDFILTERUTIL_H