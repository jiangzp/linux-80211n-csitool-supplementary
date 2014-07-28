function rets = extractCSIData(fileName)


tic;
csi_trace = read_bf_file(fileName,'lagacy');

csi_traceAll = [csi_trace{:}];
clear csi_trace;
csi_source_addr = [csi_traceAll.source_addr];
uniqueaddrs = unique(csi_source_addr);

rets =[];

clearvars -except rets uaddr csi_traceAll uaddr uniqueaddrs fileName csi_source_addr;
for uaddr = 1:length(uniqueaddrs)
    ret = struct;
    index = csi_source_addr == uniqueaddrs(uaddr);
    csi_traceS = [csi_traceAll(index)];
    ct_agc = [csi_traceS.agc]';
    if isfield(csi_traceS(1),'client_sequence')
        ct_sequence = [csi_traceS.client_sequence]';
        loopIndex = find(abs(diff(ct_sequence))>50000);
        for m = 1:length(loopIndex)
            ct_sequence(loopIndex(m)+1:end) = ct_sequence(loopIndex(m)+1:end) - ct_sequence(loopIndex(m)+1);
            ct_sequence(loopIndex(m)+1:end) = ct_sequence(loopIndex(m)+1:end) + ct_sequence(loopIndex(m))+1;
        end
        
        clear m loopIndex;
    end
    %ct_perm = [csi_traceS.perm]';
    %ct_perm =[ct_perm(1:3:end) ct_perm(2:3:end) ct_perm(3:3:end)];
    ct_timeStamp = [csi_traceS.timestamp_low]';
    ct_timeStamp = ct_timeStamp - ct_timeStamp(1);
    ct_frequency = round(1e6/median(diff(ct_timeStamp)));
    ct_rate = [csi_traceS.rate]';
    ct_Ntx = [csi_traceS.Ntx]';
    ct_Nrx = [csi_traceS.Nrx]';
    
    %ct_noise = [csi_traceS.noise]';
    rssix1 = [csi_traceS.rssi_a]';
    rssix2 = [csi_traceS.rssi_b]';
    rssix3 = [csi_traceS.rssi_c]';
    
    ct_rssi_total = db(dbinv(rssix1)+dbinv(rssix2)+dbinv(rssix3),'pow')-44- ct_agc;
    %ct_rssi = db([dbinv(rssix1) dbinv(rssix2) dbinv(rssix3)],'pow')-44- ct_agc*ones(1,3);
    
    ratesDiff = abs(diff(ct_rate));
    diffpeaks = find(ratesDiff>0);
    if ~isempty(diffpeaks) % multiple tx rate
        diffpeaks(:,2) = circshift(diffpeaks(:,1),[-1 0 ]);
        diffpeaks = [ [ 0 diffpeaks(1,1) ]; diffpeaks ;];
        diffpeaks(end,2) = length(ct_rate);
        diffpeaks(:,1) = diffpeaks(:,1) +1;
    else % only one rate
        diffpeaks = [1 length(ct_rate)];
    end
    
    segments = {};
    for i = 1:size(diffpeaks,1)
        if diff(diffpeaks(i,:)) > 5000
            segment = diffpeaks(i,1):5000:diffpeaks(i,2);
            segment(2,:) = circshift(segment(1,:),[0 -1]);
            segment(2,:) = segment(2,:) - 1;
            segment(:,end) = [];
            segment = [segment [segment(2,end)+1; diffpeaks(i,2)]];
            segment = segment';
            segments = [segments ; segment];
        else
            segments = [segments ; diffpeaks(i,:)];
        end
    end

    segments = cell2mat(segments);
    ct_csibase = zeros(length(csi_traceS),30*max(ct_Ntx)*max(ct_Nrx));
    for i = 1:size(segments,1)
        
        ct_csi = [csi_traceS(segments(i,1):segments(i,2)).csi];
        if size(ct_csi,1) == 1
            if ct_Nrx(segments(i,1)) == 1
                ct_csibase(segments(i,1):segments(i,2),1:30) = squeeze(ct_csi(1,1:1:end,:));
        else if ct_Nrx(segments(i,1)) == 2
                ct_csibase(segments(i,1):segments(i,2),1:30) = squeeze(ct_csi(1,1:2:end-1,:));
                ct_csibase(segments(i,1):segments(i,2),31:60) = squeeze(ct_csi(1,2:2:end,:));
            else if ct_Nrx(segments(i,1)) == 3
                    ct_csibase(segments(i,1):segments(i,2),1:30) = squeeze(ct_csi(1,1:3:end-2,:));
                    ct_csibase(segments(i,1):segments(i,2),31:60) = squeeze(ct_csi(1,2:3:end-1,:));
                    ct_csibase(segments(i,1):segments(i,2),61:90) = squeeze(ct_csi(1,3:3:end,:));
                end
            end

            end
        end
        
        if size(ct_csi,1)>1
            ct_csibase(segments(i,1):segments(i,2),91:120) = squeeze(ct_csi(2,1:3:end-2,:));
            ct_csibase(segments(i,1):segments(i,2),121:150) = squeeze(ct_csi(2,2:3:end-1,:));
            ct_csibase(segments(i,1):segments(i,2),151:180) = squeeze(ct_csi(2,3:3:end,:));
        end
        
        if size(ct_csi,1)>2
            ct_csibase(segments(i,1):segments(i,2),181:210) = squeeze(ct_csi(3,1:3:end-2,:));
            ct_csibase(segments(i,1):segments(i,2),211:240) = squeeze(ct_csi(3,2:3:end-1,:));
            ct_csibase(segments(i,1):segments(i,2),241:270) = squeeze(ct_csi(3,3:3:end,:));
        end
    end
    
    ret.csi = ct_csibase;
    clear ct_csiAll ct_csibase;
    ret.avgFreq = ct_frequency;
    ret.rate = ct_rate;
    ret.sourceAddr = uniqueaddrs(uaddr);
    ret.rss = ct_rssi_total;
    ret.client_seq = ct_sequence;
    ret.timestamp = ct_timeStamp;
    ret.filePath = fileName;
    
    rets.source{uaddr} = ret;
    clearvars -except rets uaddr csi_traceAll uaddr uniqueaddrs fileName csi_source_addr;
end

toc;

