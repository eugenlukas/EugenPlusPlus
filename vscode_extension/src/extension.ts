import * as path from 'path';
import { workspace, ExtensionContext } from 'vscode';
import { LanguageClient, LanguageClientOptions, ServerOptions } from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: ExtensionContext) {
    
    const config = workspace.getConfiguration('eugenppLsp');
    const configuredPath = config.get<string>('executablePath') || 'lsp-executable';

    const serverOptions: ServerOptions = {
        command: configuredPath,
        args: ['--stdio']
    };

    const clientOptions: LanguageClientOptions = {
        documentSelector: [{ scheme: 'file', language: 'eugenpp' }]
    };

    client = new LanguageClient('EugenppLspServer', 'eugen++ LSP Server', serverOptions, clientOptions);
    client.start();
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
}
