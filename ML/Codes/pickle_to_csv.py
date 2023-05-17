import os
import sys
import pickle
from datetime import datetime
import re
import pandas as pd
import numpy as np
from tqdm import tqdm
import matplotlib.pyplot as plt

GPS_KEYWORDS_LIST = ['Latitude: ', '(*10^-7) Longitude: ', '(*10^-7) Altitude: ', '(mm) Satellite-in-view: ', 'timing for this set: ']
GPS_KEYWORDS_LENGTH = [len(keyword) for keyword in GPS_KEYWORDS_LIST]

MPU6050_KEYWORDS = ['Ax: ', 'Ay: ', 'Az: ', 'gx: ', 'gy: ', 'gz: ', 'temp: ', 'timing for this set: ']
MPU6050_KEYWORDS_LENGTH = [len(x) for x in MPU6050_KEYWORDS]

###########################
######## Functions ########
###########################
def process_gps_datapoints(gps_datapoints):
    """
    gps_datapoints = gps_data['GPS Datapoints']
    """
    gps_datapoints = gps_datapoints[0]
    indices = [gps_datapoints.find(keyword) for keyword in GPS_KEYWORDS_LIST]
    # Latitude, Longtiude, Altitude, Satellite-in-view, timing for this set
    gps_datapoints_list = []
    for i, index in enumerate(indices):
        if i == len(indices) - 1:
            gps_datapoints_list.append(float(gps_datapoints[index + GPS_KEYWORDS_LENGTH[i]:].strip()))
        else:
            gps_datapoints_list.append(float(gps_datapoints[index + GPS_KEYWORDS_LENGTH[i]:indices[i+1]].strip()))
    return gps_datapoints_list

def process_gps(gps):
    """
    gps = data['Split Circuit']['GPS']
    """
    processed_gps_data = []
    gps_accounter = gps['GPS Accounter']
    gps_datapoints = gps['GPS Datapoints']
    gps_loopSpeedArr = gps['GPS LoopSpeedArr'][0]
    gps_uploadSpeedArr = gps['GPS UploadSpeedArr'][0]
    
    processed_gps_datapoints = process_gps_datapoints(gps_datapoints)
    
    processed_gps_data.append(gps_accounter)
    processed_gps_data.extend(processed_gps_datapoints)
    processed_gps_data.extend([gps_loopSpeedArr, gps_uploadSpeedArr])
    
    new_gps_df = pd.DataFrame([processed_gps_data],
                             columns = ['accounter', 'latitude', 'longitude', 'altitude', 'satelliteInView', 'timingForThisSet', 'LoopSpeed', 'UploadSpeed'])
    return new_gps_df

def process_one_set_of_datapoint(output_set):
    indexes = [output_set.find(keyword) for keyword in MPU6050_KEYWORDS]
    df_row = []
    curr_data_index = int(output_set[:indexes[0]].strip())
    df_row.append(curr_data_index) # append in the index of the new input
    for i, index in enumerate(indexes):
        if i == len(indexes) - 1:
            x = float(output_set[index+MPU6050_KEYWORDS_LENGTH[i]:].strip())
            df_row.append(x)
        else:
            x = float(output_set[index+MPU6050_KEYWORDS_LENGTH[i]: indexes[i+1]].strip())
            df_row.append(x)
    return df_row

def process_mpu6050(mpu6050, timeDifference):
    """
    mpu6050_output = split_circuit_data['MPU6050']
    """
    mpu6050_df = pd.DataFrame(columns = ['accounter', 'LoopSpeedArr', 'UploadSpeedArr', 'set_index', 'Ax', 'Ay', 'Az', 'gx', 'gy', 'gz', 'temp', 'timingForThisSet', 'timeDifference'])
    accounter = mpu6050['MPU6050 Accounter']
    mpu6050_datapoints = mpu6050['MPU6050 Datapoints'][0]
    mpu6050_loopSpeedArr = mpu6050['MPU6050 LoopSpeedArr'][0]
    mpu6050_uploadSpeedArr = mpu6050['MPU6050 UploadSpeedArr'][0]
    mpu6050_output_sets = mpu6050_datapoints.split('Set: ')[1:]
    
    for output in mpu6050_output_sets:
        data = [accounter, mpu6050_loopSpeedArr,mpu6050_uploadSpeedArr]
        datapoint = process_one_set_of_datapoint(output)
        data.extend(datapoint)
        data.append(timeDifference)
        new_df = pd.DataFrame([data], 
                              columns = ['accounter', 'LoopSpeedArr', 'UploadSpeedArr', 'set_index', 'Ax', 'Ay', 'Az', 'gx', 'gy', 'gz', 'temp', 'timingForThisSet', 'timeDifference'])
        mpu6050_df = pd.concat([mpu6050_df, new_df], ignore_index = True)
    return mpu6050_df

# overall function to read split circuit
def process_split_ciruit_data(split_circuit_data):
    """
    split_circuit_data = data['Split Circuit']
    """
    gps_df = pd.DataFrame(columns = ['accounter', 'latitude', 'longitude', 'altitude', 'satelliteInView', 'timingForThisSet', 'LoopSpeed', 'UploadSpeed'])
    mpu6050_df = pd.DataFrame(columns = ['accounter', 'LoopSpeedArr', 'UploadSpeedArr', 'set_index', 'Ax', 'Ay', 'Az', 'gx', 'gy', 'gz', 'temp', 'timingForThisSet', 'timeDifference'])

    keys = split_circuit_data.keys()
    for key in keys:
        if key == 'GPS':
            gps_data = split_circuit_data['GPS']
            processed_gps_data = process_gps(gps_data)
            gps_df = pd.concat([gps_df, processed_gps_data])
            
        elif key == 'GPS Button':
            continue
            
        elif key == 'MPU6050':
            mpu6050_output = split_circuit_data['MPU6050']
            processed_mpu6050_output = process_mpu6050(mpu6050_output, None)
            mpu6050_df = pd.concat([mpu6050_df, processed_mpu6050_output])
            #print('Extracted MPU6050 Data')
    return gps_df, mpu6050_df

def process_file(filename, dataset_cat):
    """
    Converts target file from pickle to csv
    dataset_cat: adl or fall
    """
    filename = f"../Datasets/raw/{dataset_cat}/{filename}.pkl"

    full_gps_df = pd.DataFrame(columns = ['accounter', 'latitude', 'longitude', 'altitude', 'satelliteInView', 'timingForThisSet', 'LoopSpeed', 'UploadSpeed'])
    full_mpu6050_df = pd.DataFrame(columns = ['accounter', 'LoopSpeedArr', 'UploadSpeedArr', 'set_index', 'Ax', 'Ay', 'Az', 'gx', 'gy', 'gz', 'temp', 'timingForThisSet', 'timeDifference'])
    
    curr_data = pd.read_pickle(filename)
    print("Total Accounters", len(curr_data))
    for wave in tqdm(curr_data):
        gps_df, mpu6050_df = process_split_ciruit_data(wave)
        full_gps_df = pd.concat([full_gps_df, gps_df], ignore_index=True)
        full_mpu6050_df = pd.concat([full_mpu6050_df, mpu6050_df], ignore_index=True)
        
    print("GPS DF:", full_gps_df.shape)
    print("MPU6050 DF:", full_mpu6050_df.shape)
    
    return full_gps_df, full_mpu6050_df

def save_processed_file(processed_df, filename_to_save, dataset_cat, kind):
    final_dir = f"../Datasets/curated/{dataset_cat}/{kind}_{filename_to_save}.csv"
    processed_df.to_csv(final_dir, index=False)
    print("File saved at:", final_dir)

if __name__ == "__main__":
    filename = sys.argv[1] # filename to process e.g. "data_2023-05-17_14-00-54"
    dataset_cat = sys.argv[2] 
    gps_df, mpu6050_df = process_file(filename, dataset_cat)
    save_processed_file(gps_df, filename, dataset_cat, "gps")
    save_processed_file(mpu6050_df, filename, dataset_cat, "mpu6050")
    print("Files saved successfully!")


