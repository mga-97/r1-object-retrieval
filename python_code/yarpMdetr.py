import cv2
import numpy as np
import time
import torch
torch.set_grad_enabled(False)
from PIL import Image
import torchvision.transforms as T
from collections import defaultdict
from python_code.models.model import mdetr_resnet101

import yarp

# Initialise YARP
yarp.Network.init()

class YarpMdetr(yarp.RFModule):
    
    def configure(self, rf):
        
        # Generic configs
        self.period = rf.find('period').asFloat32() if rf.check('period') else 0.1
        self.module_name = rf.find('module_name').asString() if rf.check('module_name') else 'YarpMdetr'

        self.image_w = rf.find('image_width').asInt32() if rf.check('image_width') else 640
        self.image_h = rf.find('image_height').asInt32() if rf.check('image_height') else 480
        self.min_prob = rf.find('min_confidence').asFloat32() if rf.check('min_confidence') else 0.85

        # Opening ports
        self.cmd_port = yarp.Port()
        commandPortName = rf.find('command_port').asString() if rf.check('command_port') else '/command:i' 
        self.cmd_port.open('/' + self.module_name + commandPortName)
        print('{:s} opened'.format('/' + self.module_name + commandPortName))
        self.attach(self.cmd_port)

        self._input_image_port = yarp.BufferedPortImageRgb()
        imageInPortName = rf.find('input_image_port').asString() if rf.check('input_image_port') else '/image:i'
        self._input_image_port.open('/' + self.module_name + imageInPortName)
        print('{:s} opened'.format('/' + self.module_name + imageInPortName))

        self._output_image_port = yarp.Port()
        imageOutPortName = rf.find('output_image_port').asString() if rf.check('output_image_port') else '/image:o' 
        self._output_image_port.open('/' + self.module_name + imageOutPortName)
        print('{:s} opened'.format('/' + self.module_name + imageOutPortName))

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
        self.model, postprocessors = torch.hub.load('ashkamath/mdetr:main', 'mdetr_resnet101', pretrained=True, return_postprocessor=True)
        self.postprocessors = postprocessors
        self.model.cuda()
        self.model.eval()
        
        self.caption = ''
        self.caption_coords = ''        

        return True


    def respond(self, command, reply):
        if command.get(0).asString() == 'caption':
            print('Command \'caption\' received')
            self.caption = command.get(1).asString()
            reply.addString('caption state activated: ' + self.caption)
        elif command.get(0).asString() == 'where':
            print('Command \'where\' received')
            self.caption = command.get(1).asString()
            time.sleep(1)
            reply.addString(self.caption_coords)
        else:
            print('Command {:s} not recognized'.format(command.get(0).asString()))
            reply.addString('Command {:s} not recognized'.format(command.get(0).asString()))
        return True

    def get_max_prob_coord(self, out_bbox, probs):
        if probs.size(dim=-1)>0:
            mask = probs == probs.max()
            bbox = out_bbox[mask]
            x, y, _, _ = bbox.unbind(-1)
            x_=x.item() * self.image_w
            y_=y.item() * self.image_h
            self.caption_coords = str(x_) + ' ' + str(y_)

    
    def interruptModule(self):
        print('Interrupt function')
        self.cmd_port.interrupt()
        self._input_image_port.interrupt()
        self._output_image_port.close()
        return True

    
    def getPeriod(self):
        return self.period

    
    def transform(self, im):   
        transform = T.Compose([
            T.ToTensor(),
            T.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),])
        return transform(im)

    
    def box_cxcywh_to_xyxy(self, x):
        x_c, y_c, w, h = x.unbind(-1)
        b = [(x_c - 0.5 * w), (y_c - 0.5 * h),
             (x_c + 0.5 * w), (y_c + 0.5 * h)]
        return torch.stack(b, dim=-1)
    
    
    def rescale_bboxes(self, out_bbox, size):
        img_w, img_h = size
        b = self.box_cxcywh_to_xyxy(out_bbox)
        b = b * torch.tensor([img_w, img_h, img_w, img_h], dtype=torch.float32)
        return b
    
    
    def plot_results_yarp(self ,pil_img, scores, boxes, labels,masks=None):
        self.np_image = np.array(pil_img)
        for s, (xmin, ymin, xmax, ymax), l in zip(scores, boxes.tolist(), labels):
            xmin, ymin, xmax, ymax = map(int, [xmin, ymin, xmax, ymax])
            cv2.rectangle(self.np_image, (xmin, ymin), (xmax, ymax), (0, 255, 0), 2)
            cv2.putText(self.np_image, l, (xmin, ymin - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
        return np.array(pil_img)


    def plot_inference(self, im, caption):
        im = Image.fromarray(im)
        img = self.transform(im).unsqueeze(0).cuda()
        memory_cache = self.model(img, [caption], encode_and_save=True)
        outputs = self.model(img, [caption], encode_and_save=False, memory_cache=memory_cache)
        probas = 1 - outputs['pred_logits'].softmax(-1)[0, :, -1].cpu()
        keep = probas > self.min_prob
        out_bbox = outputs['pred_boxes'].cpu()[0, keep]
        bboxes_scaled = self.rescale_bboxes(out_bbox, im.size)
        positive_tokens = (outputs["pred_logits"].cpu()[0, keep].softmax(-1) > 0.1).nonzero().tolist()
        predicted_spans = defaultdict(str)
        for tok in positive_tokens:
            item, pos = tok
            if pos < 255:
                span = memory_cache["tokenized"].token_to_chars(0, pos)
                predicted_spans[item] += " " + caption[span.start:span.end]
        labels = [predicted_spans[k] for k in sorted(list(predicted_spans.keys()))]
        self.plot_results_yarp(im, probas[keep], bboxes_scaled, labels, masks=None)

        self.get_max_prob_coord(out_bbox, probas[keep])


    def updateModule(self):
        received_image = self._input_image_port.read()
        self._in_buf_image.copy(received_image)   
        assert self._in_buf_array.__array_interface__['data'][0] == self._in_buf_image.getRawImage().__int__()
        frame = self._in_buf_array
        # print(self.caption + ': ' + self.caption_coords)
        self.plot_inference(frame, self.caption)
        self._out_buf_array[:,:] = self.np_image
        self._output_image_port.write(self._out_buf_image)
        return True


if __name__ == '__main__':

    rf = yarp.ResourceFinder()
    rf.setVerbose(True)
    rf.setDefaultContext("yarpMdetr")
    rf.setDefaultConfigFile("yarpMdetr_R1_SIM.ini");

    ap = YarpMdetr()
    ap.runModule(rf)




