// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';
import * as fs from 'fs';

// A relevant sample is https://github.com/microsoft/vscode-extension-samples/tree/main/decorator-sample
// create a decorator type that we use to decorate large numbers
const evenDecorationType = vscode.window.createTextEditorDecorationType({
	backgroundColor: 'blue'
});

const oddDecorationType = vscode.window.createTextEditorDecorationType({
	backgroundColor: 'green'
});

const uiDecorationType = vscode.window.createTextEditorDecorationType({
	backgroundColor: 'green'
});

const cryptoDecorationType = vscode.window.createTextEditorDecorationType({
	backgroundColor: 'red'
});

const networkDecorationType = vscode.window.createTextEditorDecorationType({
	backgroundColor: 'blue'
});

export const colorDocument = (textEditor: vscode.TextEditor): void =>
{
	let activeEditor = vscode.window.activeTextEditor;
	if (!activeEditor) {
		return;
	}

	const lang = textEditor.document.languageId;
	console.log(`document: ${textEditor.document.fileName}, lines: ${textEditor.document.lineCount}, lang: ${lang}`);
	let fileName = textEditor.document.fileName.split('/').pop();

	const uiLines: vscode.DecorationOptions[] = [];
	const cryptoLines: vscode.DecorationOptions[] = [];
	const networkLines: vscode.DecorationOptions[] = [];


	//We'll start with coloring according to the true labels
	let projLinesFile = fs.readFileSync('/home/moshe/dx2021_names/astminer/dataset/7zip_lines.jsonl').toString('utf-8'); 
    let projLines = projLinesFile.split("\n");
    console.log(`${projLines.length} lines`);

	projLines.forEach((element, index) => {
		try {
			if (element.length > 0){
				let snip = JSON.parse(element);
				if (snip['file'] == fileName){
					switch (snip['map_label']){
						case "crypto":
							cryptoLines.push({ range: new vscode.Range(snip['start_line'] - 1, 0, snip['end_line'], 0)});
							break;
						case "network":
							networkLines.push({ range: new vscode.Range(snip['start_line'] - 1, 0, snip['end_line'], 0)});
							break;
						case "GUI":
							uiLines.push({ range: new vscode.Range(snip['start_line'] - 1, 0, snip['end_line'], 0)});
							break;
					}
				}
			}
		} catch (error) {
			console.log(`Error ${error} while parsing ${element} at index ${index}`);
		}
	});

	activeEditor.setDecorations(uiDecorationType, uiLines);
	activeEditor.setDecorations(networkDecorationType, networkLines);	
	activeEditor.setDecorations(cryptoDecorationType, cryptoLines);	
};


// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {

	// Use the console to output diagnostic information (console.log) and errors (console.error)
	// This line of code will only be executed once when your extension is activated
	console.log('Congratulations, your extension "codepainter" is now active!');

	// The command has been defined in the package.json file
	// Now provide the implementation of the command with registerCommand
	// The commandId parameter must match the command field in package.json
	let disposable = vscode.commands.registerCommand('codepainter.colorize', () => {
		// The code you place here will be executed every time your command is executed
		// Display a message box to the user
		// vscode.window.showInformationMessage('CodePainter has been REALLY activated!');
		// vscode.window.visibleTextEditors.forEach(i => colorDocument(i));
		if (vscode.window.activeTextEditor){
			colorDocument(vscode.window.activeTextEditor);
		}
	});

	context.subscriptions.push(disposable);
}

// This method is called when your extension is deactivated
export function deactivate() {}
