pushd cmd\exampleapp\protonappui

CALL npm install yarn

CALL .\node_modules\.bin\yarn install
CALL .\node_modules\.bin\yarn build

popd


pushd cmd\exampleapp

go build
exampleapp.exe --self-test

popd
