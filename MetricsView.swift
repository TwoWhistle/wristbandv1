//
//  MetricsView.swift
//  wristbandV1
//
//  Created by Ryan Yue on 3/8/25.
//

import SwiftUI

struct MetricsView: View {
    @ObservedObject var bleManager: BLEManager
    
    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("Vital Signs")) {
                    HStack {
                        Text("SpO₂")
                        Spacer()
                        Text("\(bleManager.computedSpO2, specifier: "%.1f") %")
                            .foregroundColor(.blue)
                    }
                    HStack {
                        Text("Heart Rate")
                        Spacer()
                        Text("\(bleManager.computedHeartRate, specifier: "%.1f") BPM")
                            .foregroundColor(.blue)
                    }
                    HStack {
                        Text("Resp Rate")
                        Spacer()
                        Text("\(bleManager.computedRespRate, specifier: "%.1f") br/min")
                            .foregroundColor(.blue)
                    }
                    HStack {
                        Text("HRV")
                        Spacer()
                        Text("\(bleManager.computedHRV, specifier: "%.1f") ms")
                            .foregroundColor(.blue)
                    }
                    HStack {
                        Text("PTT")
                        Spacer()
                        Text("\(bleManager.computedPTT, specifier: "%.3f") s")
                            .foregroundColor(.blue)
                    }
                }
                
                Section(header: Text("ECG Features")) {
                    HStack {
                        Text("QRS Duration")
                        Spacer()
                        let qrsValue = bleManager.computedECGFeatures["QRS"] ?? 0
                        Text("\(qrsValue, specifier: "%.3f") s")
                            .foregroundColor(.green)
                    }
                    HStack {
                        Text("ST Amplitude")
                        Spacer()
                        let stValue = bleManager.computedECGFeatures["ST"] ?? 0
                        Text("\(stValue, specifier: "%.3f")")
                            .foregroundColor(.green)
                    }
                }
                
                Section(header: Text("SCD41 Sensor")) {
                    HStack {
                        Text("CO₂")
                        Spacer()
                        Text("\(bleManager.lastCO2, specifier: "%.1f") ppm")
                            .foregroundColor(.red)
                    }
                    HStack {
                        Text("Temperature")
                        Spacer()
                        Text("\(bleManager.lastTemperature, specifier: "%.2f") °C")
                            .foregroundColor(.red)
                    }
                    HStack {
                        Text("Humidity")
                        Spacer()
                        Text("\(bleManager.lastHumidity, specifier: "%.2f") %")
                            .foregroundColor(.red)
                    }
                }
                
                // New Blood Pressure Section
                Section(header: Text("Blood Pressure")) {
                    HStack {
                        Text("Systolic")
                        Spacer()
                        Text("\(bleManager.computedSystolicBP, specifier: "%.1f") mmHg")
                            .foregroundColor(.purple)
                    }
                    HStack {
                        Text("Diastolic")
                        Spacer()
                        Text("\(bleManager.computedDiastolicBP, specifier: "%.1f") mmHg")
                            .foregroundColor(.purple)
                    }
                }
            }
            .navigationTitle("Live Metrics")
        }
    }
}

struct MetricsView_Previews: PreviewProvider {
    static var previews: some View {
        MetricsView(bleManager: BLEManager())
    }
}

