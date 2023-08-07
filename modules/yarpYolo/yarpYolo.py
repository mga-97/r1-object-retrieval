import sys
from threading import Lock
import numpy as np
import cv2
import yarp
from ultralytics import YOLO
from ultralytics.utils.plotting import Annotator


# Initialise YARP
yarp.Network.init()

class YarpYolo(yarp.RFModule):
    
    def configure(self, rf):
        
        # Generic configs
        self.period = rf.find('period').asFloat32() if rf.check('period') else 0.1
        
        self.image_w = rf.find('image_width').asInt32() if rf.check('image_width') else 640
        self.image_h = rf.find('image_height').asInt32() if rf.check('image_height') else 480
        self.min_conf = rf.find('min_confidence').asFloat32() if rf.check('min_confidence') else 0.95

        # Opening ports
        self.cmd_port = yarp.Port()
        commandPortName = rf.find('command_port').asString() if rf.check('command_port') else '/yarpYolo/command/rpc' 
        self.cmd_port.open(commandPortName)
        print('{:s} opened'.format(commandPortName))
        self.attach(self.cmd_port)

        self._input_image_port = yarp.BufferedPortImageRgb()
        imageInPortName = rf.find('input_image_port').asString() if rf.check('input_image_port') else '/yarpYolo/image:i'
        self._input_image_port.open(imageInPortName)
        print('{:s} opened'.format(imageInPortName))

        self._output_image_port = yarp.Port()
        imageOutPortName = rf.find('output_image_port').asString() if rf.check('output_image_port') else '/yarpYolo/image:o' 
        self._output_image_port.open(imageOutPortName)
        print('{:s} opened'.format(imageOutPortName))

        self.output_coords_port = yarp.BufferedPortBottle()
        coordsOutPortName = rf.find('bbox_coord_port').asString() if rf.check('bbox_coord_port') else '/yarpYolo/bbox_coords:o' 
        self.output_coords_port.open(coordsOutPortName)
        print('{:s} opened'.format(coordsOutPortName))

        # Preparing images
        print('Preparing input image...')
        self._in_buf_array = np.ones((self.image_h, self.image_w, 3), dtype=np.uint8)
        self._in_buf_image = yarp.ImageRgb()
        self._in_buf_image.resize(self.image_w, self.image_h)
        self._in_buf_image.setExternal(self._in_buf_array, self._in_buf_array.shape[1], self._in_buf_array.shape[0])
        print('... ok \n')

        print('Preparing output image...')
        self._out_buf_image = yarp.ImageRgb()
        self._out_buf_image.resize(self.image_w, self.image_h)
        self._out_buf_array = np.zeros((self.image_h, self.image_w, 3), dtype=np.uint8)
        self._out_buf_image.setExternal(self._out_buf_array, self._out_buf_array.shape[1], self._out_buf_array.shape[0])
        print('... ok \n')

        # Defining model
        self.model = YOLO("yolov8s.pt")
        
        self.label_num = -1 
        self.x_bbox = 0.0
        self.y_bbox = 0.0  
        self.lock = Lock() 

        self.label_dict = {
            "person" : 0, 
            "bicycle" : 1, 
            "car" : 2,
            "motorcycle" : 3, 
            "airplane" : 4, 
            "bus" : 5, 
            "train" : 6, 
            "truck" : 7, 
            "boat" : 8, 
            "traffic light" : 9, 
            "fire hydrant" : 10,  
            "stop sign" : 11, 
            "parking meter" : 12,  
            "bench" : 13,  
            "bird" : 14,  
            "cat" : 15,  
            "dog" : 16, 
            "horse" : 17,  
            "sheep" : 18,  
            "cow" : 19,  
            "elephant" : 20,  
            "bear" : 21,  
            "zebra" : 22,  
            "giraffe" : 23,  
            "backpack" : 24,  
            "umbrella" : 25,  
            "handbag" : 26,  
            "tie" : 27,  
            "suitcase" : 28,  
            "frisbee" : 29,  
            "skis" : 30,  
            "snowboard" : 31,  
            "sports ball" : 32,  
            "kite" : 33,  
            "baseball bat" : 34,  
            "baseball glove" : 35,  
            "skateboard" : 36,  
            "surfboard" : 37,  
            "tennis racket" : 38,  
            "bottle" : 39,  
            "wine glass" : 40,  
            "cup" : 41,  
            "fork" : 42,  
            "knife" : 43,  
            "spoon" : 44,  
            "bowl" : 45,  
            "banana" : 46, 
            "apple" : 47,  
            "sandwich" : 48,  
            "orange" : 49,  
            "broccoli" : 50,  
            "carrot" : 51,  
            "hot dog" : 52,  
            "pizza" : 53,
            "donut" : 54,  
            "cake" : 55,  
            "chair" : 56,  
            "couch" : 57,  
            "potted plant" : 58,  
            "bed" : 59,  
            "dining table" : 60,  
            "toilet" : 61,  
            "tv" : 62,  
            "laptop" : 63,  
            "mouse" : 64,  
            "remote" : 65,  
            "keyboard" : 66,  
            "cell phone" : 67,  
            "microwave" : 68, 
            "oven" : 69,  
            "toaster" : 70,  
            "sink" : 71,  
            "refrigerator" : 72,  
            "book" : 73,  
            "clock" : 74,  
            "vase" : 75,  
            "scissors" : 76,  
            "teddy bear" : 77, 
            "hair drier" : 78,  
            "toothbrush" : 79
        }

        return True

    def respond(self, command, reply):
        if command.get(0).asString() == 'detect':
            print('Command \'detect\' received')
            self.label_num = -1
        elif command.get(0).asString() == 'label':
            print('Command \'label\' received')
            label = command.get(1).asString()
            if label in self.label_dict:
                self.label_num = self.label_dict[label]
                reply.addString('labeling: ' + str(label))
            else:
                reply.addString('cannot label (not in the available list): ' + str(label))
        elif command.get(0).asString() == 'where':
            print('Command \'where\' received')
            self.lock.acquire()
            self.x_bbox = 0.0
            self.y_bbox = 0.0
            label = command.get(1).asString()
            if label in self.label_dict:
                self.label_num = self.label_dict[label]
            else:
                reply.addString('cannot label (not in the available list): ' + str(label))
                return True
            self.plot_inference(self._in_buf_array, self.label_num)
            if self.x_bbox == 0.0:
                reply.addString('not found')
            else:
                reply.addString(label + ' is here: ' + str(self.x_bbox) + ' ' + str(self.y_bbox))
                bout = self.output_coords_port.prepare()
                bout.clear()
                bout.addFloat32(self.x_bbox)
                bout.addFloat32(self.y_bbox)
                self.output_coords_port.write()
            self.lock.release()
        elif command.get(0).asString() == 'help':
            print('Command \'help\' received')
            reply.addVocab32('many')
            reply.addString('detect : starts detecting everything in the input image')
            reply.addString('label <something> : identify "something" in input image')
            reply.addString('where <something> : identify "something" in input image and return its pixel coords')
            reply.addString('help : get this list')           
        else:
            print('Command {:s} not recognized'.format(command.get(0).asString()))
            reply.addString('Command {:s} not recognized'.format(command.get(0).asString()))
        
        if reply.size()==0:
            reply.addVocab32('ack')
        return True

    
    def interruptModule(self):
        print('Interrupt function')
        self.cmd_port.close()
        self._input_image_port.close()
        self._output_image_port.close()
        self.output_coords_port.close()
        return True

    
    def getPeriod(self):
        return self.period


    def box_label(self, image, box, label='', color=(128, 128, 128), txt_color=(255, 255, 255)):
        lw = max(round(sum(image.shape) / 2 * 0.003), 2)
        p1, p2 = (int(box[0]), int(box[1])), (int(box[2]), int(box[3]))
        cv2.rectangle(image, p1, p2, color, thickness=lw, lineType=cv2.LINE_AA)
        if label:
            tf = max(lw - 1, 1)  # font thickness
            w, h = cv2.getTextSize(label, 0, fontScale=lw / 3, thickness=tf)[0]  # text width, height
            outside = p1[1] - h >= 3
            p2 = p1[0] + w, p1[1] - h - 3 if outside else p1[1] + h + 3
            cv2.rectangle(image, p1, p2, color, -1, cv2.LINE_AA)  # filled
            cv2.putText(image,
                        label, (p1[0], p1[1] - 2 if outside else p1[1] + h + 2),
                        0,
                        lw / 3,
                        txt_color,
                        thickness=tf,
                        lineType=cv2.LINE_AA)


    def plot_bboxes(self, image, boxes, labels=[], colors=[], score=True, conf=None):
        #Define COCO Labels
        if labels == []:
            labels = {0: u'__background__', 1: u'person', 2: u'bicycle',3: u'car', 4: u'motorcycle', 5: u'airplane', 6: u'bus', 7: u'train', 8: u'truck', 9: u'boat', 10: u'traffic light', 11: u'fire hydrant', 12: u'stop sign', 13: u'parking meter', 14: u'bench', 15: u'bird', 16: u'cat', 17: u'dog', 18: u'horse', 19: u'sheep', 20: u'cow', 21: u'elephant', 22: u'bear', 23: u'zebra', 24: u'giraffe', 25: u'backpack', 26: u'umbrella', 27: u'handbag', 28: u'tie', 29: u'suitcase', 30: u'frisbee', 31: u'skis', 32: u'snowboard', 33: u'sports ball', 34: u'kite', 35: u'baseball bat', 36: u'baseball glove', 37: u'skateboard', 38: u'surfboard', 39: u'tennis racket', 40: u'bottle', 41: u'wine glass', 42: u'cup', 43: u'fork', 44: u'knife', 45: u'spoon', 46: u'bowl', 47: u'banana', 48: u'apple', 49: u'sandwich', 50: u'orange', 51: u'broccoli', 52: u'carrot', 53: u'hot dog', 54: u'pizza', 55: u'donut', 56: u'cake', 57: u'chair', 58: u'couch', 59: u'potted plant', 60: u'bed', 61: u'dining table', 62: u'toilet', 63: u'tv', 64: u'laptop', 65: u'mouse', 66: u'remote', 67: u'keyboard', 68: u'cell phone', 69: u'microwave', 70: u'oven', 71: u'toaster', 72: u'sink', 73: u'refrigerator', 74: u'book', 75: u'clock', 76: u'vase', 77: u'scissors', 78: u'teddy bear', 79: u'hair drier', 80: u'toothbrush'}
        #Define colors
        if colors == []:
            colors = [(89, 161, 197),(67, 161, 255),(19, 222, 24),(186, 55, 2),(167, 146, 11),(190, 76, 98),(130, 172, 179),(115, 209, 128),(204, 79, 135),(136, 126, 185),(209, 213, 45),(44, 52, 10),(101, 158, 121),(179, 124, 12),(25, 33, 189),(45, 115, 11),(73, 197, 184),(62, 225, 221),(32, 46, 52),(20, 165, 16),(54, 15, 57),(12, 150, 9),(10, 46, 99),(94, 89, 46),(48, 37, 106),(42, 10, 96),(7, 164, 128),(98, 213, 120),(40, 5, 219),(54, 25, 150),(251, 74, 172),(0, 236, 196),(21, 104, 190),(226, 74, 232),(120, 67, 25),(191, 106, 197),(8, 15, 134),(21, 2, 1),(142, 63, 109),(133, 148, 146),(187, 77, 253),(155, 22, 122),(218, 130, 77),(164, 102, 79),(43, 152, 125),(185, 124, 151),(95, 159, 238),(128, 89, 85),(228, 6, 60),(6, 41, 210),(11, 1, 133),(30, 96, 58),(230, 136, 109),(126, 45, 174),(164, 63, 165),(32, 111, 29),(232, 40, 70),(55, 31, 198),(148, 211, 129),(10, 186, 211),(181, 201, 94),(55, 35, 92),(129, 140, 233),(70, 250, 116),(61, 209, 152),(216, 21, 138),(100, 0, 176),(3, 42, 70),(151, 13, 44),(216, 102, 88),(125, 216, 93),(171, 236, 47),(253, 127, 103),(205, 137, 244),(193, 137, 224),(36, 152, 214),(17, 50, 238),(154, 165, 67),(114, 129, 60),(119, 24, 48),(73, 8, 110)]
        
        for box in boxes:
            if self.label_num >= 0 and int(box[-1]) != self.label_num:
                continue
            
            if score : #add score in label if score=True
                label = labels[int(box[-1])+1] + " " + str(round(100 * float(box[-2]),1)) + "%"
            else :
                label = labels[int(box[-1])+1]
        
            if box[-2] > conf:  #filter every box under conf threshold if conf threshold setted
                color = colors[int(box[-1])]
                self.box_label(image, box, label, color)

        self.imageYolo = image
    

    def get_max_prob_coord(self, boxes, conf):
        max_conf = conf
        x = self.x_bbox
        y = self.y_bbox
        for box in boxes:
            if int(box[-1]) != self.label_num:
                continue
            if box[-2] > max_conf:
                max_conf = box[-2]
                x = (float(box[0]) + float(box[2]))/2
                y = (float(box[1]) + float(box[3]))/2  
        self.x_bbox = x
        self.y_bbox = y


    def plot_inference(self, frame, label_num):
        img = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = self.model.predict(img, verbose=False)
        #boxes.data = ( top left coords, bottom rights coords, score, label_num)
        self.plot_bboxes(frame, results[0].boxes.data, conf=self.min_conf)
        self.get_max_prob_coord(results[0].boxes.data, conf=self.min_conf)
 

    def updateModule(self):
        self.lock.acquire()
        received_image = self._input_image_port.read()
        self._in_buf_image.copy(received_image)   
        assert self._in_buf_array.__array_interface__['data'][0] == self._in_buf_image.getRawImage().__int__()
        frame = self._in_buf_array
        self.plot_inference(frame, self.label_num)
        self._out_buf_array[:,:] = self.imageYolo
        self._output_image_port.write(self._out_buf_image)
        self.lock.release()
        return True


if __name__ == '__main__':
    
    rf = yarp.ResourceFinder()
    rf.setVerbose(True)
    rf.setDefaultContext("yarpYolo")
    conffile = rf.find("from").asString()
    if not conffile:
        print('Using default conf file')
        rf.setDefaultConfigFile('yarpYolo_R1.ini')
    else:
        rf.setDefaultConfigFile(rf.find("from").asString())

    rf.configure(sys.argv)

    # Run module
    ap = YarpYolo()
    try:
        ap.runModule(rf)
    finally:
        print('Closing yarpYolo...')
        ap.interruptModule()
