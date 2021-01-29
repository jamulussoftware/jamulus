#!/usr/bin/python3

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
