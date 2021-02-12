#!/usr/bin/python3

import os
import sys
import json


print('Number of arguments:', len(sys.argv), 'arguments.')
print('Argument List:', str(sys.argv))

checkoutbranchname=sys.argv[1]

jsonfilename = "io.jamulus.Jamulus.json"
print("read {}".format(jsonfilename))
with open(jsonfilename,'r') as file:
    data = json.load(file)

if data['modules'][1]['name'] != 'jamulus':
    print('error')
    raise Exception('unexpected format of file')
else:
    print("change branch to '{}'".format(checkoutbranchname))
    data['modules'][1]['sources'][0]['branch'] = checkoutbranchname

print("write {}".format(jsonfilename))
with open(jsonfilename,'w') as file:
    json.dump(data, file, indent=2)



#helper function: set github variable and print it to console
def set_github_variable(varname, varval):
    print("{}='{}'".format(varname, varval)) #console output
    print("::set-output name={}::{}".format(varname, varval))

#set github-available variables
jamulus_buildversionstring = os.environ['jamulus_buildversionstring'] 
flatpak_name = "jamulus_{:}_flatpak".format(jamulus_buildversionstring)
set_github_variable("flatpak_name", flatpak_name)
flatpak_bundle = "{:}.flatpak".format(flatpak_name)
set_github_variable("flatpak_bundle", flatpak_bundle)