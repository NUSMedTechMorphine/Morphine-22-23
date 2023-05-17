import firebase_admin
from firebase_admin import db
from datetime import datetime
import time
import json
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from vae import *
import pickle
import os

# Connection to Firebase
cred_obj = firebase_admin.credentials.Certificate("../Morphine2.json")
default_app = firebase_admin.initialize_app(cred_obj, {
  'databaseURL':'https://morphine-64cdd-default-rtdb.asia-southeast1.firebasedatabase.app/'
  })
USERS_DATA = db.reference("/Users Data/Token UID:XvIeVwC7M0QN0qW15FNYO2e5BJ93")
FIREBASE_PREDICTION_PATH = db.reference("/Users Data/Token UID:XvIeVwC7M0QN0qW15FNYO2e5BJ93/Split Circuit/MPU6050/MPU6050 Fall/")

# Process GPS Data
GPS_KEYWORDS_LIST = ['Latitude: ', '(*10^-7) Longitude: ', '(*10^-7) Altitude: ', '(mm) Satellite-in-view: ', 'timing for this set: ']
GPS_KEYWORDS_LENGTH = [len(keyword) for keyword in GPS_KEYWORDS_LIST]

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

# process MPU6050
MPU6050_KEYWORDS = ['Ax: ', 'Ay: ', 'Az: ', 'gx: ', 'gy: ', 'gz: ', 'temp: ', 'timing for this set: ']
MPU6050_KEYWORDS_LENGTH = [len(x) for x in MPU6050_KEYWORDS]

## function to process one datapoint
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
            print('Extracted GPS Data')
            
        elif key == 'GPS Button':
            print("GPS Button:", split_circuit_data['GPS Button'])
            
        elif key == 'MPU6050':
            mpu6050_output = split_circuit_data['MPU6050']
            processed_mpu6050_output = process_mpu6050(mpu6050_output, None)
            mpu6050_df = pd.concat([mpu6050_df, processed_mpu6050_output])
            print('Extracted MPU6050 Data')
    return gps_df, mpu6050_df

# MAIN FUNCTION to read data from Google Firebase
def read_data():
    data = USERS_DATA.get()
    split_circuit_data = data['Split Circuit']
    gps_df, mpu6050_df = process_split_ciruit_data(split_circuit_data)
    return gps_df, mpu6050_df

def pivot_df(df, num_new_cols = 20):
    df = df.copy()
    for num_new_rows in range(1, num_new_cols + 1):
        new_rows = pd.DataFrame([[0,0,0,0,0,0] for i in range(num_new_rows)], columns = ['Ax','Ay','Az','gx','gy','gz'])
        temp_df = pd.concat([
            new_rows, df[['Ax','Ay','Az','gx','gy','gz']]
        ], axis=0).\
        reset_index(drop=True).\
        rename(columns={
            'Ax' : f'Ax{num_new_rows}',
            'Ay' : f'Ay{num_new_rows}',
            'Az' : f'Az{num_new_rows}',
            'gx' : f'gx{num_new_rows}',
            'gy' : f'gy{num_new_rows}',
            'gz' : f'gz{num_new_rows}'
        })
        df = pd.concat([df, temp_df], axis = 1)
    df = df.dropna().iloc[20:,:].reset_index(drop=True)
    return df

def predict(wave_data, t1=8.6396, t2=0.5):
    """
    Actual:
    This function make use of the variational autoencoder model to predict one wave of data - 20 datapoints.
    The model will predict whether each of this 20 datapoints is ADL or Fall from the reconstruction error threshold, t1.
    If the percentage of datapoints identified as anomalous within a wave is more than a certain threshold, t2, we will then classify it as Fall, else ADL.
    Note that 0 <= t2 <= 1 and t1 >= 0
    For now:
    t1 is threshold to classify one datapoint based on the y-axis of accelerometer 
    t2 is threshold to classify a wave - 0.5 i.e. if >= 50% of the datapoints are anomalous, this wave will be classified as fall
    """
    return "Fall detected" if np.sum(np.abs(wave_data.Ay) > t1) >= t2 * wave_data.shape[0] else "Normal"

def predict_vae(wave_data, prev_wave_data, vae_model, scaler_model, t1=10.32485739914194, t2=0.5):
    """
    This function make use of the variational autoencoder model to predict one wave of data - 20 datapoints.
    The model will predict whether each of this 20 datapoints is ADL or Fall from the reconstruction error threshold, t1.
    If the percentage of datapoints identified as anomalous within a wave is more than a certain threshold, t2, we will then classify it as Fall, else ADL.
    Note that 0 <= t2 <= 1 and t1 >= 0
    """
    wave_data = wave_data[["Ax", "Ay", "Az", "gx", "gy", "gz"]]
    wave_data = pivot_df(pd.concat([prev_wave_data, wave_data], axis = 0).reset_index(drop=True))
    wave_data_norm = scaler_model.transform(wave_data)
    wave_predictions = vae_model.predict(wave_data_norm)
    wave_data_mae = np.sum(np.abs(wave_data_norm - wave_predictions), axis = 1)
    return "Fall Detected" if np.sum(wave_data_mae > t1) >= t2 * wave_data.shape[0] else "Normal"
    

def write_to_firebase(prediction, prediction_path=FIREBASE_PREDICTION_PATH):
    """ Write results to firebase """
    if prediction.lower() == "fall detected":
        prediction_path.update({"0":"Fall detected"})
    elif prediction.lower() == "normal":
        prediction_path.update({"0":"Normal"})

if __name__ == "__main__":
    # Instantiating VAE model
    input_dim = 126
    latent_space_dim = 8
    output_dim = 126

    encoder = get_encoder(input_dim=input_dim, latent_space_dim=latent_space_dim)
    sampler = get_sampler(latent_space_dim=latent_space_dim)
    decoder = get_decoder(latent_space_dim=latent_space_dim, output_dim=output_dim)

    vae = VAE(encoder, sampler, decoder, beta = 0.01)
    vae.compile(optimizer = 'adam')

    model_name = "vae_windowing_loss_beta_0_01"
    vae.load_weights("./../Model/weights/" + model_name)

    # Instantiating MinMaxScaler
    scaler = pickle.load(open('./../Model/weights/scaler_windowing.pkl', 'rb'))

    gps_df, mpu6050_df = read_data()
    prev_wave_data = mpu6050_df.copy()[["Ax", "Ay", "Az", "gx", "gy", "gz"]]

    while True:
        gps_df, mpu6050_df = read_data()
        print("GPS and MPU6050 Dataframe Shape:", gps_df.shape, mpu6050_df.shape)
        prediction = predict_vae(wave_data=mpu6050_df, prev_wave_data = prev_wave_data, vae_model=vae, scaler_model = scaler)
        print("Prediction:", prediction.title())
        prev_wave_data = mpu6050_df.copy()[["Ax", "Ay", "Az", "gx", "gy", "gz"]]
        write_to_firebase(prediction)
        print("Updated Firebase!")
        print()
