classdef Goertz

    properties
        target_frequency
        sample_frequency
        coeff
        cosine
        sine
        Q1
        Q2
    end

    methods

        function obj = Goertz(target_frequency, sample_frequency)
            obj.target_frequency = target_frequency;
            obj.sample_frequency = sample_frequency;
            omega = (2.0 * pi * target_frequency) / sample_frequency;
            obj.cosine = cos(omega);
            obj.coeff = 2.0 * cos(omega);
            obj.sine = sin(omega);
            obj.Q1 = 0;
            obj.Q2 = 0;
        end

        function obj = processSample(obj, newSample)
            Q0 = obj.coeff * obj.Q1 - obj.Q2 + newSample;
            obj.Q2 = obj.Q1;
            obj.Q1 = Q0;
        end
        
        function obj = reset(obj)
            obj.Q1 = 0;
            obj.Q2 = 0;
        end
        
        function magsquared = calcMagSquared(obj)
            magsquared = obj.Q1 * obj.Q1 + obj.Q2 * obj.Q2 - obj.coeff * obj.Q1 * obj.Q2;
        end

        function mag = calcMagnitude(obj)
            mag = sqrt(obj.calcMagSquared());
        end
        
        function real = calcRead(obj)
            real = obj.Q1-obj.Q2*obj.cosine;
        end
        
        function imag = calcImag(obj)
            imag = obj.Q2*obj.sine;
        end
        
        function purity = calcPurity(obj, N)
            purity = 2*obj.calcMagSquared()/N;
        end
        

    end

end
