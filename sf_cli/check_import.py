import os
import sys
try:
    import pip
except ImportError as e:
    print("The lack of PIP library :Skip the inspection!")

third_party_libraries = ["requests" , "requests_toolbelt"]

for lib in third_party_libraries:
    try:
        exec("import {0}".format(lib))
    except ImportError as e:
        print("The lack of %s library!"%e.name)
        print("Waiting for the installation!")
        install_list = ["install",lib]
        pip.main(install_list)
        pass                                              
