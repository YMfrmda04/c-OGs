#include <iostream>
#include <string>
#include <filesystem>
#include <memory>
#include <vector>
#include <algorithm>
#include <random>

namespace fs = std::filesystem;

class FileSystemComponent {
public:
    virtual std::string getName() const = 0;
    virtual void listContents() const = 0;
    virtual bool changeDirectory(const std::string& newDir) = 0;
    virtual int getSize() const = 0;
    virtual bool isDirectory() const = 0;
};

class File : public FileSystemComponent {
public:
    File(const fs::path& path) : filePath(path) {}
    File(const fs::path& path, int fileSize) : filePath(path), size(fileSize) {}

        std::string getName() const override {
        return filePath.filename().string();
    }

    int getSize() const override {
        if (fs::is_regular_file(filePath)) {
            return static_cast<int>(fs::file_size(filePath));
        }
    }

    void listContents() const override {
        std::cout << "File: " << getName() << " (" << getSize() << " bytes)" << std::endl;
    }

    bool isDirectory() const override {
        return false;
    }

    bool changeDirectory(const std::string& newDir) override {
        return false;
    }

private:
    fs::path filePath;
    std::string name;
    int size;
};

class Directory : public FileSystemComponent {
public:
    Directory(const fs::path& path) : currentDirectory(path) {
        //contents.clear(); // Clear the existing contents
        for (const auto& entry : fs::directory_iterator(currentDirectory)) {
            if (entry.is_directory()) {
                auto subDir = std::make_shared<Directory>(entry.path());
                contents.push_back(subDir);
            } else {
                auto file = std::make_shared<File>(entry.path());
                contents.push_back(file);
            }
        }
    }

    std::string getName() const override {
        return currentDirectory.filename().string();
    }

    int getSize() const override {
        int totalSize = 0;
        for (const auto& entry : fs::directory_iterator(currentDirectory)) {
            if (entry.is_regular_file()) {
                totalSize += static_cast<int>(fs::file_size(entry));
            }
        }
        return totalSize;
    }


    void listContents() const override {
        for (const auto& entry : contents) {
            if (entry->isDirectory()) {
                std::cout << "Directory: " << entry->getName() << " " << entry->getSize() << std::endl;
            } else {
                std::cout << "File: " << entry->getName() << " " << entry->getSize() << std::endl;
            }
        }
    }

    bool isDirectory() const override {
        return true;
    }

    bool changeDirectory(const std::string& newDir) override {
        fs::path newDirectory = currentDirectory / newDir;
        if (fs::is_directory(newDirectory)) {
            currentDirectory = newDirectory;
            return true;
        }
        return false;
    }

    const std::vector<std::shared_ptr<FileSystemComponent>>& getContents() const {
        return contents;
    }

    void sortContentsBySize() {
        // Sort contents by size
        std::sort(contents.begin(), contents.end(),
                  [](const std::shared_ptr<FileSystemComponent>& a, const std::shared_ptr<FileSystemComponent>& b) {
                      return a->getSize() < b->getSize();
                  }
        );
    }

    void sortContentsByName() {
        // Sort contents by size
        std::sort(contents.begin(), contents.end(),
                  [](const std::shared_ptr<FileSystemComponent>& a, const std::shared_ptr<FileSystemComponent>& b) {
                      return strcasecmp(a->getName().c_str(), b->getName().c_str()) < 0;
                      //research done for strcasecmp remember to list this later ::)(::
                  }
        );
    }


    bool createSubdirectory2(const fs::path newDir) {
        fs::path newDirectory = currentDirectory / newDir;
        if (!fs::exists(newDirectory) && fs::create_directory(newDirectory)) {
            auto subDir = std::make_shared<Directory>(newDirectory);
            contents.push_back(subDir);
            return true;
        }
        return false;
    }

    bool createSubdirectory(const fs::path newDir) {
        auto newDirectory = currentDirectory / newDir;

        for (auto item : contents){
            if(item->getName() == newDir)
                return false;
        }

        try {
            auto subDir = std::make_shared<Directory>(newDirectory);
            contents.push_back(subDir);
            return true;
        } catch (std::exception e){
            std::cerr << "Error" << e.what() << std::endl;
        }
    }



    bool createFile(const std::string newFile, int fileSize) {
        fs::path newFilePath = newFile;
        if (!fs::exists(newFilePath)) {
            auto file = std::make_shared<File>(newFile, fileSize);
            contents.push_back(file);
            return true;
        }
        return false;
    }



private:
    fs::path currentDirectory;
    mutable std::vector<std::shared_ptr<FileSystemComponent>> contents; // Make it mutable
};

class FakeFileSystem {
public:
    FakeFileSystem() : currentDirectory(std::make_shared<Directory>(fs::current_path())) {}

    void dir() {
        currentDirectory->listContents();
    }


    bool cd(const std::string& newDir) {
        return currentDirectory->changeDirectory(newDir);
    }


    void sortsize() {
        currentDirectory->sortContentsBySize();
        currentDirectory->listContents();
    }

    void sortname() {
        currentDirectory->sortContentsByName();
        currentDirectory->listContents();
    }

    void mkdir(std::string newDir){
        bool created = currentDirectory->createSubdirectory(newDir);
        if(created)
            std::cout << "Directory: " << newDir << " was created" << std::endl;
        else
            std::cout << "Directory: " << newDir << " already exists" << std::endl;
    }

    void mkfile(std::string newFile){
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> distribution(1, 1024);
        int fileSize = distribution(gen);

        bool created = currentDirectory->createFile(newFile, fileSize);
        if(created)
            std::cout << "File: " << newFile << " was created" << std::endl;
        else
            std::cout << "File: " << newFile << " already exists" << std::endl;
    }




private:
    std::shared_ptr<Directory> currentDirectory;
};

class CommandLineInterface {
public:
    explicit CommandLineInterface(FakeFileSystem& fs) : fileSystem(fs) {}

    void start() {
        std::string input;
        while (true) {
            std::cout << "Enter a command ('dir', 'cd name', 'sortsize', 'sortname', 'mkdir' 'exit'): ";
            std::getline(std::cin, input);

            if (input == "dir") {
                fileSystem.dir();
            } else if (input.substr(0, 3) == "cd ") {
                std::string newDir = input.substr(3);
                if (!fileSystem.cd(newDir)) {
                    std::cout << "Invalid directory or not a directory." << std::endl;
                }
            } else if (input == "sortsize") {
                fileSystem.sortsize();
            } else if (input == "sortname") {
                fileSystem.sortname();
            } else if(input.substr(0, 6) == "mkdir "){
                std::string newDir = input.substr(6);
                fileSystem.mkdir(newDir);
            } else if(input.substr(0, 7) == "mkfile "){
                std::string newFile = input.substr(7);
                fileSystem.mkfile(newFile);
            } else if (input == "exit") {
                break;
            } else {
                std::cout << "Invalid command. Please try again." << std::endl;
            }
        }
    }

private:
    FakeFileSystem& fileSystem;
};

int main() {
    FakeFileSystem fs;
    CommandLineInterface cli(fs);
    cli.start();
    return 0;
}
