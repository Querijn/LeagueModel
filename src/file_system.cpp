#include "file_system.hpp"

#include <map>
#include <algorithm>

std::map<std::string, File*>* m_Files = nullptr;

File * FileSystem::GetFile(const std::string& a_FilePath)
{
	std::string t_FilePath = a_FilePath;
	std::transform(t_FilePath.begin(), t_FilePath.end(), t_FilePath.begin(), ::tolower);
	if (m_Files == nullptr)
	{
		m_Files = new std::map<std::string, File*>();
		std::atexit([]()
		{
			for (auto& i : *m_Files)
				delete i.second;

			m_Files->clear();
			m_Files = nullptr;
		});
	}

	auto t_Index = m_Files->find(t_FilePath);
	if (t_Index == m_Files->end())
	{
		auto* t_File = new File(t_FilePath);
		(*m_Files)[t_FilePath] = t_File;
		return t_File;
	}

	return t_Index->second;
}

void FileSystem::CloseLoadedFiles()
{
	if (m_Files == nullptr)
		return; 

	bool t_ShouldRemove = false;
	for (auto t_FileIterator : *m_Files)
	{
		if (t_FileIterator.second->IsHandled() == false)
			continue;

		delete t_FileIterator.second;
		t_FileIterator.second = nullptr;
		t_ShouldRemove = true;
	}

	for (auto i = m_Files->cbegin(); i != m_Files->cend();)
	{
		if (i->second == nullptr)
			i = m_Files->erase(i);    
		else i++;
	}
}
