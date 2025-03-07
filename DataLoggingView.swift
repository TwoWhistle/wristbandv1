//
//  DataLoggingView.swift
//  wristbandV1
//
//  Created by Ryan Yue on 3/7/25.
//

//
//  DataLoggingView.swift
//  trackerV3
//
//  Created by Ryan Yue on 2/18/25.
//

// DataLoggingView.swift
import SwiftUI

struct DataLoggingView: View {
    @ObservedObject var bleManager: BLEManager
    @State private var selectedState: String = ""
    @State private var selectedTime: Date = Date()
    @State private var loggedData: [(Date, String)] = []
        
    var body: some View {
        VStack {
            Text("Data Logging")
                .font(.title)
                .padding()
            
            .padding()
            Button("Export Raw Data") {
                bleManager.exportData()
            }
            .padding()
            .buttonStyle(.borderedProminent)
        }
    }
    
    /*func saveToFile() {
        let fileURL = FileManager.default.temporaryDirectory.appendingPathComponent("BrainStateLog.txt")
        let content = loggedData.map { "\($0.0),\($0.1)" }.joined(separator: "\n")
        try? content.write(to: fileURL, atomically: true, encoding: .utf8)
    }
    
    func exportData() {
        let fileURL = FileManager.default.temporaryDirectory.appendingPathComponent("BrainStateLog.txt")
        let activityVC = UIActivityViewController(activityItems: [fileURL], applicationActivities: nil)
        UIApplication.shared.windows.first?.rootViewController?.present(activityVC, animated: true)
    }*/
}
