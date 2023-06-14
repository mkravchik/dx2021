package astminer.examples

import astminer.cli.*
import astminer.cli.separateToken
import astminer.cli.MethodNameExtractor
import astminer.common.*
import astminer.common.model.*
import astminer.parse.cpp.FuzzyCppParser
import astminer.paths.*
import com.google.gson.Gson
import com.google.gson.GsonBuilder
import java.io.*
import java.nio.charset.StandardCharsets
import java.nio.file.Files
import java.nio.file.Paths
import astminer.ast.DotAstStorage

// data class Sample (val project: String, val commit_id: String, val target: String, val func: String, val idx: String)
// The lines are 1-based and the last line is included
data class Sample (val project: String, val file: String, val start_line: Int, val end_line: Int, val func: String, val func_name: String?, val label: String?)
data class SampleWithFullFunc (val project: String, val file: String,
 val start_line: Int, val end_line: Int, val func: String, val func_name: String?,
  val label: String?, val file_path: String?, val full_func: String?, val begin: Int?, val end: Int?,
   val function_start_line: Int?, val function_end_line: Int?)
data class SampleSnippet (val project: String, val file: String, val start_line: Int, val end_line: Int, val snippet: String, val label: String, val map_label: String)

fun printPath(path: ASTPath){
    println("The path is $path")
    println("upwardNodes")
    path.upwardNodes.forEach { print("*"); it.prettyPrint(indent = 1, withChildren = false) }
    println("topNode")
    print("*")
    println(path.topNode.prettyPrint(indent = 1, withChildren = false))
    println("downwardNodes")
    path.downwardNodes.forEach { print("*"); it.prettyPrint(indent = 1, withChildren = false) }
}

fun countLinesinStream(content: InputStream) : Int{
    val reader = content.bufferedReader()
    val lines = reader.lines()
    return lines.count().toInt();
}

fun findDifference(first: String, second: String)
{
    //if lengths differ - print them and return
    if (first.length != second.length) {
        println("The lengths of the strings are different")
        println("The first string is ${first.length} characters long")
        println(first)
        println("The second string is ${second.length} characters long")
        println(second)
        return
    }

    //Convert both strings to char array and compare them until the first difference is found
    //Print the difference, its location, and return
    val firstCharArray = first.toCharArray()
    val secondCharArray = second.toCharArray()
    for (i in 0 until first.length) {
        if (firstCharArray[i] != secondCharArray[i]) {
            println("The first difference is at position $i")
            println("The first string has ${first.length-i} characters left")
            println("The second string has ${second.length-i} characters left")
            println("The first string has ${first.substring(i)}")
            println("The second string has ${second.substring(i)}")
            return
        }
    }   
}

//Retrieve paths from all JavaScript files, using an Antlr parser.
//JavaScriptMethodSplitter is used to extract individual method nodes from the compilation unit tree.
fun code2vecCMethods(split: String, window: Int, step: Int, method_label: Boolean) {
    println("method_label $method_label")

    val source = "dataset/${split}.jsonl"
    val source_lines = "dataset/${split}_lines.jsonl"
    val source_with_asts = "dataset/${split}_with_asts.jsonl"
    val outputDir = "../code2vec"
    val writer = BufferedWriter(
        OutputStreamWriter(
        FileOutputStream(source_lines), "UTF-8"))
    val writer_with_asts = BufferedWriter(
        OutputStreamWriter(
        FileOutputStream(source_with_asts), "UTF-8"))
    val gson = GsonBuilder().disableHtmlEscaping().create()
    //TODO - here I want to create a sliding window over each function instead
//    val miner = PathMiner(PathRetrievalSettings(8, 3, 0, 100000000))
    val storage = Code2VecPathStorage(split, outputDir)
    val totalLines = Files.lines(Paths.get(source), StandardCharsets.UTF_8).count()
    
    // Declare a local variable for the source file lines pointed by sample.file_path
    var sourceFilePath = ""
    //Declare sourceFileLines as an empty array of Objects
    var sourceFileLines: Array<Any> = emptyArray()

    println("There are $totalLines lines in $source")
    var cnt = 0
    File(source).forEachLine { line ->
        // wrap each line in try-catching to avoid the program crashing
        try{
            cnt += 1
            print("\r$cnt\t\t/$totalLines")
            // val sample = Gson().fromJson(line, Sample::class.java)
            val sample = Gson().fromJson(line, SampleWithFullFunc::class.java)
            // println("Sample: ${sample}")

            // There are multple cases:
            // 1. The sample.func contains the full function and we need to slice it (the original setup)
            // 2. The sample.full_func contains the full function and we need to extract the part between sample.begin and sample.end
            // 3. The sample.func contains the snippet and we need to extract the full function from the source between sample.function_start_line and sample.function_end_line
            
            var func_body: String?;
            var begin: Int? = null;
            var end: Int? = null
            // Check whether we are in the 3rd case
            if (sample.function_start_line != null && sample.function_end_line != null) {
                // Check whether we have already loaded the file
                if (sample.file_path != sourceFilePath && sample.file_path != null) {
                    // Load the file
                    sourceFilePath = sample.file_path
                    //check whether the file exists
                    if (!File(sourceFilePath).exists()) {
                        println("The file ${sourceFilePath} does not exist")
                        println("------------------------------------------------------------------------------")
                        // continue to the next line
                        return@forEachLine
                    }

                    // Read the file into an array of lines converting to String array
                    sourceFileLines = Files.lines(Paths.get(sourceFilePath), StandardCharsets.UTF_8).toArray()
                }
                // Extract the function. 
                func_body = sourceFileLines.slice(sample.function_start_line.toInt()-1..sample.function_end_line.toInt()-1).joinToString("\n")
                // println("func_body ${func_body}")
                // println("------------------------------------------------------------------------------")
                val snippet = sourceFileLines.slice(sample.start_line-1..sample.end_line-1).joinToString("\n")
                // println("snippet ${snippet}")
                // println("------------------------------------------------------------------------------")

                //strip anything but letters, numbers, and punctuation characters from the snippet
                val strippedSnippet = snippet.replace("[^a-zA-Z0-9\\p{Punct} ]".toRegex(), "")
                //same with the function
                val strippedFunc = sample.func.replace("[^a-zA-Z0-9\\p{Punct} ]".toRegex(), "")

                //compare the snippet with the sample.func, both stripped from whitespaces
                if (strippedSnippet != strippedFunc) {
                    println("The snippet is different from the sample.func in ${sourceFilePath}")
                    println("------------------------------------------------------------------------------")
                    //print where are the differences
                    findDifference(strippedSnippet, strippedFunc)

                    // continue to the next line
                    return@forEachLine
                }
                //set sample.func to the full function, sample.begin and sample .end to corresponding line numbers inside the full function
                begin = sample.start_line - sample.function_start_line + 1
                end = sample.end_line - sample.function_start_line + 1
            }
            else {
                // Check whether we are in the 2nd case
                if (sample.full_func != null) {
                    // Extract the function
                    func_body = sample.full_func
                    begin = sample.begin
                    end = sample.end
                    // println("func_body ${func_body}")
                    // println("------------------------------------------------------------------------------")
                }
                else {
                    // We are in the 1st case
                    // Extract the function
                    func_body = sample.func
                    // println("func_body ${func_body}")
                    // println("------------------------------------------------------------------------------")
                }
            }


            var label = sample.label ?: sample.project

            val fileNode = FuzzyCppParser().parseInputStream(func_body.byteInputStream(StandardCharsets.UTF_8)) ?: return@forEachLine
            val labelExtractor = MethodNameExtractor()
            var dummyParseResult = ParseResult(fileNode, sample.file)
            normalizeParseResult(dummyParseResult, splitTokens = true)
            val labeledParseResults = labelExtractor.toLabeledData(dummyParseResult)
            // Print the label of the function (there should be only one item)
            if (method_label){
                if (labeledParseResults.count() > 0){
                    label = labeledParseResults.first().label
                    // println("\nmethod label is ${label}")
                }
                else {
                    println("\nCan't extract method from ${func_body}")
                    if (sample.func_name != null && sample.func_name != ""){
                        label = separateToken(sample.func_name)
                        println("\nUsing ${sample.func_name} - ${label}")
                    }
                    else{
                        println(" skipping")
                        return@forEachLine
                    }
                }
            }
            // println("label $label")
            // println("------------------------------------------------------------------------------")

            fileNode.preOrder().forEach { 
                // println("Node is ${it.getToken()}")
                // it.prettyPrint(withChildren=false)
                it.setNormalizedToken(separateToken(it.getToken())) 
            }

            // Dump Function AST
            // val dotStorage = DotAstStorage(outputDir)
            // dotStorage.store(fileNode, label) 
            // dotStorage.close()

            // I want to calculate a number of sliding windows over the function
            val reader = func_body.byteInputStream(StandardCharsets.UTF_8).bufferedReader()
            val lines = reader.lines().toArray()

            val fileLines = lines.count().toInt();
            // println("There are $fileLines lines in this function")

            val winStep = if (window == 0) fileLines else step
            var startLine: Int = 1
            if (begin != null){
                startLine = begin
            }

            while (startLine < fileLines){
                var endLine = if (window != 0 && startLine + window <= fileLines) startLine + window else fileLines
                if (end != null) {
                    endLine = end
                }

                val miner = PathMiner(PathRetrievalSettings(8, 3, startLine, endLine))
                val paths = miner.retrievePaths(fileNode)
                // println("startLine ${startLine} endLine ${endLine} fileLines ${fileLines}")
                // println("\n" + paths.size.toString() + " paths between $startLine - $endLine: ")
    //            paths.forEach{
    //                //println("The path is $it")
    //                printPath(it)
    //            }

    //            if (split == "train" && paths.isEmpty()) return@forEachLine
                if (paths.isNotEmpty()){
                    storage.store(
                        LabeledPathContexts(
                            label,
                            paths.map { toPathContext(it) { node -> node.getNormalizedToken() } })
                    )
                    val code_snip = lines.sliceArray(startLine - 1 until endLine)
                    val snip = SampleSnippet(project = sample.project, file = sample.file, start_line = sample.start_line + startLine - 1,
                        end_line = sample.start_line + endLine - 1, snippet = code_snip.joinToString(separator = "\n "),
                        label = label, map_label= sample.label ?: sample.project)

                    gson.toJson(snip, writer);
                    writer.newLine();
                    
                    //write the original line into the source_with_asts file using writer_with_asts
                    writer_with_asts.write(line);
                    writer_with_asts.newLine();
                }
                else {
                    println(" No paths between $startLine - $endLine at ${sourceFilePath} (line $cnt)")
                }
                if (begin != null){
                    break // there is a single sample
                }

                startLine += winStep
            }
            System.gc()
        }
        catch (e: Exception) {
            println("Exception ${e.message} in ${sourceFilePath}")
            println("------------------------------------------------------------------------------")
        }
    }
    storage.close()
    writer.close()
    writer_with_asts.close()
    println("\n$source completed\n")
}
