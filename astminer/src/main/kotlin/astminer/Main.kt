package astminer

import astminer.cli.*
import astminer.examples.code2vecCMethods

//fun main(args: Array<String>) = ProjectPreprocessor().main(args)

fun main(args: Array<String>) {
	if(args[0] == "preprocess"){
		ProjectPreprocessor().main(args.copyOfRange(1,args.size))
	}else if(args[0] == "parse"){
		ProjectParser().main(args.copyOfRange(1,args.size))
	}else if(args[0] == "pathContexts"){
		PathContextsExtractor().main(args.copyOfRange(1,args.size))
	}else if(args[0] == "code2vec"){
		Code2VecExtractor().main(args.copyOfRange(1,args.size))
	}else{
		code2vecCMethods(args[0])
	}
}