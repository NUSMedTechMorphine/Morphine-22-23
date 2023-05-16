# Importing necessary libraries
import numpy as np
import pandas as pd
import os
import tensorflow as tf
import keras
from keras.layers import Input, Dense, Reshape, Concatenate, Flatten, Lambda, Reshape, Dropout
from tensorflow.keras import backend as K
from tensorflow.keras.models import Model
from sklearn import preprocessing
import random

# Instantiating VAE model
latent_space_dim = 2
def sampling(args):
    z_mean, z_log_sigma = args
    epsilon = K.random_normal(shape=(latent_space_dim,), mean=0., stddev=1.)
    return_value = z_mean + K.exp(z_log_sigma) * epsilon
    #print(z_mean)
    return return_value

inputs = Input((6,))
layer1 = Dense(6, activation = 'relu')(inputs)
layer2 = Dense(200, activation = 'relu')(layer1) # Changed from 3
layer3 = Dense(100, activation = 'relu')(layer2) # Changed from 3
layer4 = Dense(60, activation = 'relu')(layer3)
layer5 = Dense(30, activation = 'relu')(layer4)
z_mean = Dense(latent_space_dim)(layer5)
z_log_sigma = Dense(latent_space_dim)(layer5)
encoder = keras.Model(inputs, [z_mean, z_log_sigma], name="encoder")
sampler_input1 = keras.Input(shape=(latent_space_dim,))
sampler_input2 = keras.Input(shape=(latent_space_dim,))
latent_sample = Lambda(sampling, output_shape = (latent_space_dim,))([sampler_input1, sampler_input2])
sampler = keras.Model([sampler_input1, sampler_input2], latent_sample)
latent_inputs = keras.Input(shape=(latent_space_dim,))
layer6 = Dense(20, activation = 'relu')(latent_inputs)
layer7 = Dense(30, activation = 'relu')(layer6)
layer8 = Dense(50, activation = 'relu')(layer7)
layer9 = Dense(100, activation = 'relu')(layer8)
layer10 = Dense(200, activation = 'relu')(layer9)
decoder_outputs = Dense(6, activation = 'sigmoid')(layer10)
decoder = keras.Model(latent_inputs, decoder_outputs, name="decoder")

class VAE(Model):
    def __init__(self, encoder, sampler, decoder, beta = 1, **kwargs):
        super().__init__(**kwargs)
        self.encoder = encoder
        self.sampler = sampler
        self.decoder = decoder
        self.beta = beta
        self.total_loss_tracker = keras.metrics.Mean(name="total_loss")
        self.reconstruction_loss_tracker = keras.metrics.Mean(
            name="reconstruction_loss"
        )
        self.kl_loss_tracker = keras.metrics.Mean(name="kl_loss")

    @property
    def metrics(self):
        return [
            self.total_loss_tracker,
            self.reconstruction_loss_tracker,
            self.kl_loss_tracker,
        ]

    def train_step(self, data):
        data = data[0]
        with tf.GradientTape() as tape:
            z_mean, z_log_sigma = self.encoder(data)
            z_log_var = 2 * z_log_sigma
            z = self.sampler([z_mean, z_log_var])
            reconstruction = self.decoder(z)
            reconstruction_loss = tf.reduce_mean(keras.losses.binary_crossentropy(data, reconstruction))
            kl_loss = -0.5 * (1 + z_log_var - tf.square(z_mean) - tf.exp(z_log_var))
            kl_loss = tf.reduce_mean(kl_loss)
            total_loss = reconstruction_loss + self.beta * kl_loss
        grads = tape.gradient(total_loss, self.trainable_weights)
        self.optimizer.apply_gradients(zip(grads, self.trainable_weights))
        self.total_loss_tracker.update_state(total_loss)
        self.reconstruction_loss_tracker.update_state(reconstruction_loss)
        self.kl_loss_tracker.update_state(kl_loss)
        return {
            "loss": self.total_loss_tracker.result(),
            "reconstruction_loss": self.reconstruction_loss_tracker.result(),
            "kl_loss": self.kl_loss_tracker.result(),
        }
    def test_step(self, data):
        if isinstance(data, tuple):
            #print(f"Ran. data: {data}")
            data = data[0]

        z_mean, z_log_sigma = self.encoder(data)
        z_log_var = 2 * z_log_sigma
        z = self.sampler([z_mean, z_log_var])
        reconstruction = self.decoder(z)
        reconstruction_loss = tf.reduce_mean(keras.losses.binary_crossentropy(data, reconstruction))
        kl_loss = -0.5 * (1 + z_log_var - tf.square(z_mean) - tf.exp(z_log_var))
        kl_loss = tf.reduce_mean(kl_loss)
        total_loss = reconstruction_loss + self.beta * kl_loss
        return {
            "loss": total_loss,
            "reconstruction_loss": reconstruction_loss,
            "kl_loss": kl_loss,
        }
    
    
    def call(self, data):
        z_mean, z_log_sigma = self.encoder(data)
        z_log_var = 2 * z_log_sigma
        z = self.sampler([z_mean, z_log_var])
        reconstruction = self.decoder(z)
        reconstruction_loss = tf.reduce_mean(keras.losses.binary_crossentropy(data, reconstruction))
        kl_loss = -0.5 * (1 + z_log_var - tf.square(z_mean) - tf.exp(z_log_var))
        kl_loss = tf.reduce_mean(kl_loss)
        total_loss = reconstruction_loss + self.beta * kl_loss
        self.add_metric(kl_loss, name='kl_loss', aggregation='mean')
        self.add_metric(total_loss, name='total_loss', aggregation='mean')
        self.add_metric(reconstruction_loss, name='reconstruction_loss', aggregation='mean')
        return reconstruction
   
vae = VAE(encoder, sampler, decoder, beta = 0.01)
vae.compile(optimizer = 'adam')

os.chdir("C:/Users/ernest.liu/Documents/git/Morphine-22-23/ML/Model/weights/") # Modify this
model_name = "vae_fixed_loss_beta_0_01"
vae.load_weights(model_name)


# Calling out predictions

DATASETS_PATH = "C:/Users/ernest.liu/Documents/git/Morphine-22-23/ML/Datasets/19-01-2023/"
os.chdir(DATASETS_PATH)
os.listdir()
adl_files = [file_name for file_name in os.listdir() if ".csv" in file_name and "trial" in file_name]
fall_files = [file_name for file_name in os.listdir() if ".csv" in file_name and "fall" in file_name]
trial_data_4 = pd.read_csv(DATASETS_PATH+'trial_data_4.csv').iloc[:,1:]
trial_data_5 = pd.read_csv(DATASETS_PATH+'trial_data_5.csv').iloc[:,1:]
fall_simulation_2 = pd.read_csv(DATASETS_PATH+'fall_simulation_2.csv').iloc[:,1:]
fall_simulation_3 = pd.read_csv(DATASETS_PATH+'fall_simulation_3.csv').iloc[:,1:]
# Only take in accelerometer and gyroscope data
trial_data_4 = trial_data_4[['Ax','Ay','Az','gx','gy','gz']]
trial_data_5 = trial_data_5[['Ax','Ay','Az','gx','gy','gz']]
fall_simulation_2 = fall_simulation_2[['Ax','Ay','Az','gx','gy','gz']]
fall_simulation_3 = fall_simulation_3[['Ax','Ay','Az','gx','gy','gz']]
train_data = pd.concat([trial_data_4, trial_data_5]).reset_index(drop=True)
test_data = pd.concat([fall_simulation_2, fall_simulation_3]).reset_index(drop=True)
scaler = preprocessing.MinMaxScaler()
scaler.fit(train_data)
train_data_norm_presplit = scaler.transform(train_data)
test_data_norm = scaler.transform(test_data)
valid_ratio = 0.2
valid_indices = sorted(random.sample(list(range(train_data_norm_presplit.shape[0])), round(train_data_norm_presplit.shape[0] * valid_ratio)))
valid_data = train_data.iloc[valid_indices, :].reset_index(drop = True)
valid_data_norm = train_data_norm_presplit[valid_indices]
train_data_norm = np.delete(train_data_norm_presplit, valid_indices, axis = 0)

train_predictions = vae.predict(train_data_norm)
test_predictions = vae.predict(test_data_norm)
valid_predictions = vae.predict(valid_data_norm)