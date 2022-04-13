cd astminer/
./gradlew shadowJar
./cli.sh test
./cli.sh valid
./cli.sh train

cd ../code2vec
source preprocess.sh

