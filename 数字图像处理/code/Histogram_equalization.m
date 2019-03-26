function [output] = Histogram_equalization(input_image)
%first test the image is a RGB or gray image
if numel(size(input_image)) == 3
    %this is a RGB image
    %here is just one method, if you have other ways to do the
    %equalization, you can change the following code
    r=input_image(:,:,1);
    v=input_image(:,:,2);
    b=input_image(:,:,3);
    r1 = hist_equal(r);
    v1 = hist_equal(v);
    b1 = hist_equal(b);
    output = cat(3,r1,v1,b1);    
else
    %this is a gray image
    [output] = hist_equal(input_image);
    
end

    function [output2] = hist_equal(input_channel)
    %you should complete this sub-function

    %output2=input_channel*1.2+30;
    
    %input_channel=im2double(input_channel);
    [row,col]=size(input_channel);
    [counts,x]=imhist(input_channel);
    tmp=counts;
    for i=1:256
        sum=0;
        for j=1:i
            sum=sum+counts(j);
        end
        tmp(i)=sum/(row*col);
    end
    for i=1:256
        P=find(input_channel==x(i));
        input_channel(P)=x(i)*tmp(i);
    end
    output2=input_channel;
    end
end