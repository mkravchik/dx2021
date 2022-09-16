package astminer.examples

import astminer.cli.separateToken
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

// data class Sample (val project: String, val commit_id: String, val target: String, val func: String, val idx: String)
data class Sample (val project: String, val file: String, val func: String, val label: String)
data class SampleSnippet (val project: String, val file: String, val snippet: String, val label: String)

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

//Retrieve paths from all JavaScript files, using an Antlr parser.
//JavaScriptMethodSplitter is used to extract individual method nodes from the compilation unit tree.
fun code2vecCMethods(split: String, window: Int, step: Int) {
    val source = "dataset/${split}.jsonl"
    val source_lines = "dataset/${split}_lines.jsonl"
    val outputDir = "../code2vec"
    val writer = BufferedWriter(
        OutputStreamWriter(
        FileOutputStream(source_lines), "UTF-8"))
    val gson = GsonBuilder().disableHtmlEscaping().create()
    //TODO - here I want to create a sliding window over each function instead
//    val miner = PathMiner(PathRetrievalSettings(8, 3, 0, 100000000))
    val storage = Code2VecPathStorage(split, outputDir)
    val totalLines = Files.lines(Paths.get(source), StandardCharsets.UTF_8).count()

    println("There are $totalLines lines in $source")
    var cnt = 0
    File(source).forEachLine { line ->
        cnt += 1
        print("\r$cnt\t\t/$totalLines")
        val sample = Gson().fromJson(line, Sample::class.java)
        var label = sample.project
        // Disregard the warning "Condition 'sample.label != null' is always 'true'" - it is wrong,
        // if label is missing it will be null and we want to use the project instead
        @Suppress("SENSELESS_COMPARISON")
        if (sample.label != null && sample.label != ""){
            label = sample.label
        }
        val fileNode = FuzzyCppParser().parseInputStream(sample.func.byteInputStream(StandardCharsets.UTF_8)) ?: return@forEachLine

        fileNode.preOrder().forEach { it.setNormalizedToken(separateToken(it.getToken())) }

        // I want to calculate a number of sliding windows over the function
        val reader = sample.func.byteInputStream(StandardCharsets.UTF_8).bufferedReader()
        val lines = reader.lines().toArray()

        val fileLines = lines.count().toInt();
        // println("There are $fileLines lines in this function")

        val winStep = if (window == 0) fileLines else step
        for (startLine: Int in 1..fileLines - window step winStep) {
            val endLine = if (window != 0) startLine.toInt() + window else fileLines
            val miner = PathMiner(PathRetrievalSettings(8, 3, startLine.toInt(), endLine))
            val paths = miner.retrievePaths(fileNode)

            // println(paths.size.toString() + " paths between $startLine - $endLine: ")
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
                val snip = SampleSnippet(project = sample.project, file = sample.file,
                    snippet = code_snip.joinToString(separator = "\n "), label = sample.label)

                gson.toJson(snip, writer);
                writer.newLine();
            }
        }
        System.gc()
    }
    storage.close()
    writer.close()
    println("\n$source completed\n")
}
