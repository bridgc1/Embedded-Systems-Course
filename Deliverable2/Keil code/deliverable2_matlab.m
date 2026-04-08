%% Configuration
port     = "COM7"; % use your COM#
baudrate = 115200;
device = serialport(port, baudrate);
device.Timeout = 30;     % Read timeout set to 30s
configureTerminator(device, "CR/LF");   % MCU sends \r\n
flush(device); 

%% Parameter definitions
num_points = 32;         % points per scan
num_scans  = 3;          % total scans -- can change but need to modify plot colour array
x_spacing  = 30;        % x-axis distance between scans (mm) -- can modify

y_vals = zeros(1, num_points*num_scans);
z_vals = zeros(1, num_points*num_scans);
x_vals = zeros(1, num_points*num_scans);
idx = 1;

for scan_idx = 1:num_scans
    %% Wait for MCU start character 's'
    fprintf("Waiting for button press on MCU for scan %d...\n", scan_idx);
    while true
        if device.NumBytesAvailable > 0
            line = readline(device);
            line = strtrim(string(line));
            if strlength(line) == 0 % don't read lines that are empty
                continue;
            end
            if contains(line, "s")
                break;
            end
        else
            pause(0.01);
        end
    end
    fprintf("Scan %d started!\n", scan_idx);
    
    %% Read measurements for this scan
    for point_idx = 1:num_points
        while true
            if device.NumBytesAvailable > 0
                line = readline(device);
                line = strtrim(string(line));
                if strlength(line) > 0
                    break;
                end
            else
                pause(0.01);
            end
        end
        
        fprintf("%s\n", line); % print for visual confirmation
        
        % Parse "y, z" values
        parts = split(line, ",");

        if numel(parts) == 2
            y_vals(idx) = str2double(parts{1});
            z_vals(idx) = str2double(parts{2});
            
            x_vals(idx) = (scan_idx-1)*x_spacing;
            
            idx = idx + 1;
        end
    end
end

clear device;

%% generate visual
figure;
hold on;
% 3 distinct colors for 3 scans
colors = [1 0 0;    % red
          0 1 0;    % green
          0 0 1];   % blue

for scan_idx = 1:num_scans
    indices = (scan_idx-1)*num_points + (1:num_points);
    plot3(x_vals(indices), y_vals(indices), z_vals(indices), '.-', 'Color', colors(scan_idx,:), 'MarkerSize', 13, 'LineWidth',1.3,'DisplayName', sprintf('Scan %d (x=%d mm)', scan_idx, (scan_idx-1)*x_spacing));
end

grid on;
axis equal;
xlabel('X (mm)');
ylabel('Y (mm)');
zlabel('Z (mm)');
title('3D Plot of Mapped Environment');
legend;

view(45,30);