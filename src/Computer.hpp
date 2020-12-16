#pragma once
#include <string>
#include <any>
#include <set>
#include "Security/SecuritySuite.hpp"
#include "FileSystem/FileSystemImport.hpp"
#include "lib/sol/sol.hpp"

class Computer {
    private:
        const std::string separator = "\n-----------------\n";

        std::string name;
        std::string id;
        std::string ip;
        std::set<Computer*> links;

        Folder* root = new Folder("/");
        SecuritySuite security;

        sol::state lua;

        std::map<std::string, sol::object> systemVars;
    public:
        Computer(std::string name, std::string id, std::string ip, int securityLevel) : 
            name{name}, id{id}, ip{ip}, security(securityLevel)
        {
            lua.open_libraries(sol::lib::base);

            //               |  function name   |      method definition      | instance
            lua.set_function("getSystemVariable", &Computer::getSystemVariable, this);
            lua.set_function("setSystemVariable", &Computer::setSystemVariable, this);
            lua.set_function("getFileSystem", &Computer::getFileSystem, this);

            /*
            Usertype registration
            Used for interacting with the API
            in an OOP manner.

            Example:
            root = getFileSystem()
            print(root:getTree())
            */ 
            sol::usertype<FileSystemElement> fsElementType = lua.new_usertype<FileSystemElement>("FileSystemElement");
            fsElementType.set_function("getParent", &FileSystemElement::getParent);
            fsElementType.set_function("getName", &FileSystemElement::getName);
            fsElementType.set_function("toString", &FileSystemElement::toString);

            sol::usertype<Folder> folderType = lua.new_usertype<Folder>("Folder",
                                                                        sol::constructors<Folder(std::string)>(),
                                                                        sol::base_classes, sol::bases<FileSystemElement>());

            folderType.set_function("getTree", sol::resolve<std::string()>(&Folder::getTree));
            folderType.set_function("getType", &Folder::getType);
            folderType.set_function("setParent", &Folder::setParent);
            folderType.set_function("openFolder", &Folder::getOrCreateFolder);
            // folderType.set_function("openFile", &Folder::getOrCreateFile);
            folderType.set_function("getElement", &Folder::getElement);
            folderType.set_function("getChildren", &Folder::getChildrenTable);
        }

        ~Computer() {
            delete root;
        }

        Folder* getFileSystem() {
            return root;
        }

        SecuritySuite getSecurity() {
            return security;
        }

        bool connected(Computer *computer) {
            return links.count(computer);
        }

        void addLink(Computer *computer) {
            links.insert(computer);
        }

        std::string getIP() {
            return ip;
        }

        std::string toString() {
            return name + "::" + ip;
        }

        std::string scan() {
            std::string computerAmount = std::to_string(links.size());
            std::string res = "Scanning ended. Found " + computerAmount + " computers.";

            if (links.empty()) {
                return "Scanning ended. No computers were found\n";
            }

            res += separator;
            for (auto comp : links) {
                res += comp->toString() + '\n';
            }

            return res;
        }

        void run(Executable* exe, std::vector<std::string> args) {
            sol::load_result load = lua.load_file(exe->getScript());

            if (!load.valid()) {
                sol::error err = load;
                std::cerr << "There was an error parsing file: " << exe->getName() << '\n';
                std::cerr << err.what() << '\n';
            }

            sol::function f = load;
            sol::environment execEnv(lua, sol::create, lua.globals());
            sol::set_environment(execEnv, f);

            f(sol::as_table(args));
        }


        // ============================== LUA API ============================== 

        sol::object getSystemVariable(std::string key) {
            return systemVars[key];
        }

        void setSystemVariable(std::string key, sol::object value) {
            systemVars[key] = value;
        }

        // =====================================================================
};
