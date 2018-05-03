#!/bin/bash


cd ..

path=`pwd`

mongod --shutdown --dbpath=$path/mongo_data/data